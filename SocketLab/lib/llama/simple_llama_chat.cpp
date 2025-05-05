/*
 * Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include "llama.h"
#include "server.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/*********** some variables to save globle status ***********/

static llama_model *model = nullptr;
static const llama_vocab *vocab = nullptr;
static llama_context_params ctx_params;

struct llama_chat_single_user {
    int conn = -1;
    int prev_len = 0;
    llama_context *ctx = nullptr;
    llama_sampler *smpl = nullptr;
    std::vector<char> formatted;
    std::vector<llama_chat_message> messages;
    std::shared_ptr<std::mutex> lock = std::make_shared<std::mutex>();
};

// chat_msgs is a map of user names to llama_chat_single_user objects
// this map should be protected by a lock
static std::unordered_map<std::string, llama_chat_single_user> chat_msgs;
static std::shared_mutex chat_msgs_lock;

// llama.cpp is not thread safe, so we need to lock the generate function
static std::mutex generate_lock;

/*********** some variables to save globle status ***********/

static void print_usage(int, char **argv) {
    printf("\nexample usage:\n");
    printf("\n    %s -m model.gguf\n", argv[0]);
    printf("\n");
}

static std::string generate(const std::string &prompt,
                            const llama_chat_single_user &chat_user) {
    const bool is_first = llama_kv_self_used_cells(chat_user.ctx) == 0;

    // tokenize the prompt
    const int n_prompt_tokens = -llama_tokenize(
        vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token> prompt_tokens(n_prompt_tokens);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(),
                       prompt_tokens.data(), prompt_tokens.size(), is_first,
                       true) < 0) {
        GGML_ABORT("failed to tokenize the prompt\n");
    }

    // prepare a batch for the prompt
    std::string response;
    llama_token new_token_id;
    llama_batch batch =
        llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    while (true) {
        // check if we have enough space in the context to evaluate this batch
        int n_ctx = llama_n_ctx(chat_user.ctx);
        int n_ctx_used = llama_kv_self_used_cells(chat_user.ctx);
        if (n_ctx_used + batch.n_tokens > n_ctx) {
            printf("\033[0m\n");
            fprintf(stderr, "context size exceeded\n");
            exit(0);
        }

        {
            // lock the generate lock
            // NOTE: llama_decode() is not thread safe
            std::unique_lock<std::mutex> lock(generate_lock);

            if (llama_decode(chat_user.ctx, batch)) {
                GGML_ABORT("failed to decode\n");
            }
        }

        // sample the next token
        new_token_id = llama_sampler_sample(chat_user.smpl, chat_user.ctx, -1);

        // unlock the generate lock

        // is it an end of generation?
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }

        // convert the token to a string, print it and add it to the response
        char buf[256];
        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0,
                                     true);
        if (n < 0) {
            GGML_ABORT("failed to convert token to piece\n");
        }
        std::string piece(buf, n);
        printf("%s", piece.c_str());
        fflush(stdout);
        response += piece;

        // send the piece to the client
        send_token(chat_user.conn, piece.c_str());

        // prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    // send the EOG token to the client
    send_eog_token(chat_user.conn);
    return response;
};

#ifdef __cplusplus
extern "C" {
#endif

int initialize_llama_chat(int argc, char **argv) {
    std::string model_path;
    int n_ctx = 2048;

    // parse command line arguments
    if (argc != 3 || std::string(argv[1]) != "-m") {
        print_usage(argc, argv);
        return 1;
    }
    model_path = argv[2];

    // only print errors
    llama_log_set(
        [](enum ggml_log_level level, const char *text,
           void * /* user_data */) {
            if (level >= GGML_LOG_LEVEL_ERROR) {
                fprintf(stderr, "%s", text);
            }
        },
        nullptr);

    // load dynamic backends
    ggml_backend_load_all();

    // initialize the model
    llama_model_params model_params = llama_model_default_params();

    model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model) {
        fprintf(stderr, "%s: error: unable to load model\n", __func__);
        return 1;
    }

    vocab = llama_model_get_vocab(model);

    // initialize the context
    ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_ctx;
    ctx_params.n_batch = n_ctx;

    return 0;
}

int add_chat_user(const char *user_name) {
    // lock the map to protect data
    std::unique_lock<std::shared_mutex> lock(chat_msgs_lock);

    // check if the user name is already in the map
    if (chat_msgs.find(user_name) != chat_msgs.end()) {
        fprintf(stderr, "%s: error: user name already exists: %s\n", __func__,
                user_name);
        return 1;
    }

    // initialize the context for the user
    llama_chat_single_user chat_user;
    chat_user.ctx = llama_init_from_model(model, ctx_params);
    if (!chat_user.ctx) {
        fprintf(stderr, "%s: error: failed to create the llama_context\n",
                __func__);
        return 1;
    }

    // initialize the sampler
    // NOTE: for the purpose of test, we set sampling temperature to 0.0
    chat_user.smpl =
        llama_sampler_chain_init(llama_sampler_chain_default_params());
    // clang-format off
    // llama_sampler_chain_add(chat_user.smpl, llama_sampler_init_min_p(0.05f, 1));
    // llama_sampler_chain_add(chat_user.smpl, llama_sampler_init_temp(0.8f));
    // llama_sampler_chain_add(chat_user.smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    // clang-format on
    llama_sampler_chain_add(chat_user.smpl, llama_sampler_init_temp(0.0f));
    llama_sampler_chain_add(chat_user.smpl, llama_sampler_init_greedy());

    // initialize the formatted string
    chat_user.formatted.resize(llama_n_ctx(chat_user.ctx));

    // assign some basic values
    chat_user.conn = -1; // no connection yet
    chat_msgs.insert({user_name, std::move(chat_user)});
    return 0;
}

int remove_chat_user(const char *user_name) {
    // lock the map to protect data
    std::unique_lock<std::shared_mutex> lock(chat_msgs_lock);

    // find the user in the map
    auto it = chat_msgs.find(user_name);
    if (it == chat_msgs.end()) {
        fprintf(stderr, "%s: error: user name not found: %s\n", __func__,
                user_name);
        return 1;
    }

    // free the messages
    for (auto &msg : it->second.messages) {
        free(const_cast<char *>(msg.content));
    }

    // free the context and sampler
    llama_sampler_free(it->second.smpl);
    llama_free(it->second.ctx);

    // remove the user from the map
    chat_msgs.erase(it);
    return 0;
}

void free_llama_chat() {
    // lock the map to protect data
    std::unique_lock<std::shared_mutex> lock(chat_msgs_lock);

    // if there's any user left, free them
    for (auto &it : chat_msgs) {
        // free the messages
        for (auto &msg : it.second.messages) {
            free(const_cast<char *>(msg.content));
        }

        // free the context and sampler
        llama_sampler_free(it.second.smpl);
        llama_free(it.second.ctx);
    }
    chat_msgs.clear();

    // release the lock
    lock.unlock();

    // free the model
    if (model) {
        llama_model_free(model);
        model = nullptr;
    }
}

int quest_for_response(int conn, const char *user_name,
                       const char *user_input) {
    // lock the map to protect data (read lock)
    std::shared_lock<std::shared_mutex> lock(chat_msgs_lock);

    // find the user in the map
    auto it = chat_msgs.find(user_name);
    if (it == chat_msgs.end()) {
        fprintf(stderr, "%s: error: user name not found: %s\n", __func__,
                user_name);
        return 1;
    }

    // try to lock *it->second.lock
    // if it fails, the user is already processing a request
    if (it->second.lock->try_lock() == false) {
        fprintf(stderr, "%s: error: user is already processing a request: %s\n",
                __func__, user_name);
        return 1;
    }

    // lock the user instance to protect data
    std::unique_lock<std::mutex> user_lock(*it->second.lock, std::adopt_lock);

    // directly copy the instance to avoid data race
    llama_chat_single_user chat_user = chat_msgs[user_name];

    // release the lock (read lock)
    lock.unlock();

    // assign the connection
    chat_user.conn = conn;

    // add the user input to the message list
    chat_user.messages.push_back({"user", strdup(user_input)});

    const char *tmpl = llama_model_chat_template(model, /* name */ nullptr);

    int new_len = llama_chat_apply_template(
        tmpl, chat_user.messages.data(), chat_user.messages.size(), true,
        chat_user.formatted.data(), chat_user.formatted.size());
    if (new_len > (int)chat_user.formatted.size()) {
        chat_user.formatted.resize(new_len);
        new_len = llama_chat_apply_template(
            tmpl, chat_user.messages.data(), chat_user.messages.size(), true,
            chat_user.formatted.data(), chat_user.formatted.size());
    }
    if (new_len < 0) {
        fprintf(stderr, "failed to apply the chat template\n");
        chat_user.conn = -1; // reset the connection
        return 1;
    }

    // remove previous messages to obtain the prompt to generate the response
    std::string prompt(chat_user.formatted.begin() + chat_user.prev_len,
                       chat_user.formatted.begin() + new_len);

    // generate a response
    std::string response = generate(prompt, chat_user);

    // add the response to the messages
    chat_user.messages.push_back({"assistant", strdup(response.c_str())});

    // update the previous length
    chat_user.prev_len =
        llama_chat_apply_template(tmpl, chat_user.messages.data(),
                                  chat_user.messages.size(), false, nullptr, 0);
    if (chat_user.prev_len < 0) {
        fprintf(stderr, "failed to apply the chat template\n");
        chat_user.conn = -1; // reset the connection
        return 1;
    }

    chat_user.conn = -1; // reset the connection

    // update the user instance in the map (write lock)
    std::unique_lock<std::shared_mutex> lock2(chat_msgs_lock);
    chat_msgs[user_name] = std::move(chat_user);
    lock2.unlock();

    // unlock the user instance
    user_lock.unlock();
    return 0;
}

#ifdef __cplusplus
}
#endif
