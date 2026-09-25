#include <stdio.h>

typedef struct { float weights[4]; } Model;

static float infer(const Model *model, const float state[4])
{
    float score = 0.0f;
    for (int i = 0; i < 4; ++i)
        score += model->weights[i] * state[i];
    return score;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 1;

    Model model;
    float state[4] = {1.0f, 0.5f, -1.0f, 2.0f};
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) return 1;
    if (fread(&model, sizeof model, 1, file) != 1) return 1;
    fclose(file);

    printf("score = %.2f\n", infer(&model, state));
    return 0;
}
