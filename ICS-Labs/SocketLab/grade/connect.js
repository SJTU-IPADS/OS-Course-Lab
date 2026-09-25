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

const WebSocket = require('ws');
let response = '> ';

// create a WebSocket connection to `localhost:5175`
const socket = new WebSocket('ws://localhost:5175');

// execute when the connection is established
socket.on('open', () => {
    console.log('Connected to the server.');

    // send a message to the server
    const message = {
        token: 'Hello',
        eog: true,
    };
    socket.send(JSON.stringify(message));
    console.log('Message sent:', message);
});

// execute when a message is received from the server
socket.on('message', (data) => {
    // convert to JSON object
    const message = JSON.parse(data);

    // check if the response reaches the end
    if (message.eog) {
        console.log('')
        console.log('End of message received.');
        socket.close();
        return;
    }

    // print the message to the console
    process.stdout.write(message.token);
    response += message.token;
});

// execute when the connection is closed
socket.on('close', () => {
    console.log('Connection closed.');

    // open std_answer.txt
    const fs = require('fs');
    const path = require('path');
    const filePath = path.join(__dirname, 'std_answer.txt');

    // read the content of std_answer.txt
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error('Error reading file:', err);
            console.log('Final score: 0');
            return;
        }

        // compare the response with the content of std_answer.txt
        if (response === data) {
            console.log('Final score: 100');
        } else {
            console.log('Your response: ', response);
            console.log('Expected response: ', data);
            console.log('Final score: 0');
        }
    });
});

// execute when an error occurs
socket.on('error', (error) => {
    console.error('WebSocket error:', error);
});
