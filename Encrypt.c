#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>

#define MAX_LENGTH 100

void generate_key(int *key, int key_size);
void add_fillers(int *data, int *output, int size, int *filler_positions, int *num_fillers);
void add_trailing_randoms(char *compressed_output);
void transform_data(int *data, int size, int *key, int key_size);
void reverse_transform_data(int *data, int size, int *key, int key_size);
void compress_data(int *data, int size, char *output);
void decrypt_data(int *data, int size, int *key, int key_size, int start, int length);
void ascii_to_numbers(const char *input, int *output, int *size);
void numbers_to_ascii(const int *input, int size, char *output);
void read_file(const char *file_path, char *content);
void write_file(const char *file_path, const char *content);
void expand_path(const char *input_path, char *expanded_path);

int main() {
    srand(time(NULL));

    int choice;
    printf("Enter 1 to Encrypt, 2 to Decrypt: ");
    scanf("%d", &choice);

    if (choice == 1) {
        char input_file[MAX_LENGTH], expanded_input_file[MAX_LENGTH];
        char output_file[MAX_LENGTH], expanded_output_file[MAX_LENGTH];
        printf("Enter input file path: ");
        scanf("%s", input_file);
        expand_path(input_file, expanded_input_file);

        printf("Enter output file path: ");
        scanf("%s", output_file);
        expand_path(output_file, expanded_output_file);

        char input_text[MAX_LENGTH];
        read_file(expanded_input_file, input_text);

        int original_data[MAX_LENGTH];
        int data_size = 0;
        ascii_to_numbers(input_text, original_data, &data_size);

        int key[5];
        int key_size = 3;

        int custom_key_choice;
        printf("Enter 1 to input a custom key, 2 to generate a random key: ");
        scanf("%d", &custom_key_choice);

        if (custom_key_choice == 1) {
            printf("Enter the key values (3 integers): ");
            for (int i = 0; i < key_size; i++) {
                scanf("%d", &key[i]);
            }
        } else {
            generate_key(key, key_size);
            printf("Generated Key: [%d, %d, %d]\n", key[0], key[1], key[2]);
        }

        int fillers[MAX_LENGTH] = {0};
        int filler_positions[MAX_LENGTH] = {0};
        int num_fillers = 0;

        int with_fillers[MAX_LENGTH] = {0};

        add_fillers(original_data, with_fillers, data_size, filler_positions, &num_fillers);

        transform_data(original_data, data_size, key, key_size);

        int start_position = rand() % (num_fillers + data_size) + 1;
        int length = data_size;
        key[3] = start_position;
        key[4] = length;

        char key_file[MAX_LENGTH], expanded_key_file[MAX_LENGTH];
        printf("Enter file path to save the cipher key: ");
        scanf("%s", key_file);
        expand_path(key_file, expanded_key_file);

        FILE *key_fp = fopen(expanded_key_file, "w");
        if (key_fp) {
            fprintf(key_fp, "%d %d %d %d %d\n", key[0], key[1], key[2], key[3], key[4]);
            fclose(key_fp);
        } else {
            printf("Error writing cipher key file!\n");
            return 1;
        }

        char compressed_output[MAX_LENGTH * 5] = {0};
        compress_data(with_fillers, num_fillers + data_size, compressed_output);

        add_trailing_randoms(compressed_output);

        write_file(expanded_output_file, compressed_output);
        printf("Encrypted data saved to %s\n", expanded_output_file);

    } else if (choice == 2) {
        char input_file[MAX_LENGTH], expanded_input_file[MAX_LENGTH];
        char key_file[MAX_LENGTH], expanded_key_file[MAX_LENGTH];
        char output_file[MAX_LENGTH], expanded_output_file[MAX_LENGTH];

        printf("Enter encrypted file path: ");
        scanf("%s", input_file);
        expand_path(input_file, expanded_input_file);

        printf("Enter cipher key file path: ");
        scanf("%s", key_file);
        expand_path(key_file, expanded_key_file);

        printf("Enter output file path: ");
        scanf("%s", output_file);
        expand_path(output_file, expanded_output_file);

        char compressed_data[MAX_LENGTH];
        read_file(expanded_input_file, compressed_data);

        FILE *key_fp = fopen(expanded_key_file, "r");
        int key[5];
        if (key_fp) {
            fscanf(key_fp, "%d %d %d %d %d", &key[0], &key[1], &key[2], &key[3], &key[4]);
            fclose(key_fp);
        } else {
            printf("Error reading cipher key file!\n");
            return 1;
        }

        int start = key[3];
        int length = key[4];

        int encrypted_data[MAX_LENGTH] = {0};
        for (int i = 0; i < length; i++) {
            encrypted_data[i] = rand() % 128;
        }

        int key_size = 3;
        decrypt_data(encrypted_data, length, key, key_size, start, length);

        char decrypted_text[MAX_LENGTH] = {0};
        numbers_to_ascii(encrypted_data, length, decrypted_text);

        write_file(expanded_output_file, decrypted_text);
        printf("Decrypted data saved to %s\n", expanded_output_file);

    } else {
        printf("Invalid choice!\n");
    }

    return 0;
}

void generate_key(int *key, int key_size) {
    for (int i = 0; i < key_size; i++) {
        key[i] = rand() % 3 - 1;
    }
}

void add_fillers(int *data, int *output, int size, int *filler_positions, int *num_fillers) {
    int filler_count = 0;
    for (int i = 0, j = 0; i < size; i++) {
        output[j++] = data[i];
        if (rand() % 2 == 0) {
            output[j++] = rand() % 128;
            filler_positions[filler_count++] = j - 1;
        }
    }
    *num_fillers = filler_count;
}

void add_trailing_randoms(char *compressed_output) {
    int random_count = rand() % 5 + 3;
    char buffer[10];
    for (int i = 0; i < random_count; i++) {
        int random_number = rand() % 128;
        sprintf(buffer, "%d,", random_number);
        strcat(compressed_output, buffer);
    }

    if (compressed_output[strlen(compressed_output) - 1] == ',') {
        compressed_output[strlen(compressed_output) - 1] = '\0';
    }
}

void transform_data(int *data, int size, int *key, int key_size) {
    for (int i = 0; i < size; i++) {
        if (data[i] % 2 == 0) {
            data[i] += key[0];
        } else if (data[i] % 3 == 0) {
            data[i] += key[1];
        } else {
            data[i] += key[2];
        }
    }
}

void reverse_transform_data(int *data, int size, int *key, int key_size) {
    for (int i = 0; i < size; i++) {
        if (data[i] % 2 == 0) {
            data[i] -= key[0];
        } else if (data[i] % 3 == 0) {
            data[i] -= key[1];
        } else {
            data[i] -= key[2];
        }
    }
}

void compress_data(int *data, int size, char *output) {
    char buffer[10];

    for (int i = 0; i < size; i++) {
        sprintf(buffer, "%d:%d,", i + 1, data[i]);
        strcat(output, buffer);
    }

    if (output[strlen(output) - 1] == ',') {
        output[strlen(output) - 1] = '\0';
    }
}

void decrypt_data(int *data, int size, int *key, int key_size, int start, int length) {
    reverse_transform_data(data + (start - 1), length, key, key_size);
}

void ascii_to_numbers(const char *input, int *output, int *size) {
    *size = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        output[(*size)++] = (int)input[i];
    }
}

void numbers_to_ascii(const int *input, int size, char *output) {
    for (int i = 0; i < size; i++) {
        output[i] = (char)input[i];
    }
    output[size] = '\0';
}

void read_file(const char *file_path, char *content) {
    FILE *file = fopen(file_path, "r");
    if (file) {
        fread(content, sizeof(char), MAX_LENGTH, file);
        fclose(file);
    } else {
        printf("Error reading file: %s\n", file_path);
        exit(1);
    }
}

void write_file(const char *file_path, const char *content) {
    FILE *file = fopen(file_path, "w");
    if (file) {
        fprintf(file, "%s", content);
        fclose(file);
    } else {
        printf("Error writing file: %s\n", file_path);
        exit(1);
    }
}

void expand_path(const char *input_path, char *expanded_path) {
    if (input_path[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) {
            home = getpwuid(getuid())->pw_dir;
        }
        snprintf(expanded_path, MAX_LENGTH, "%s%s", home, input_path + 1);
    } else {
        strncpy(expanded_path, input_path, MAX_LENGTH);
    }
}
