#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#define DATA_FILE "flights.json"
#define BUFFER_SIZE 256

// Prototipos
cJSON *load_json(const char *filename);
int save_json(const char *filename, cJSON *json);
void list_flights(cJSON *array);
void show_flight(cJSON *array, const char *id);
void add_flight(cJSON *array);
void edit_flight(cJSON *array, const char *id);
void delete_flight(cJSON *array, const char *id);
char *prompt(const char *msg);

int main() {
    cJSON *root = load_json(DATA_FILE);
    if (!root || !cJSON_IsArray(root)) {
        fprintf(stderr, "Error: JSON inválido o no encontrado.\n");
        return 1;
    }

    char cmd[BUFFER_SIZE];
    printf("Interfaz CLI para editar %s\n", DATA_FILE);
    printf("Comandos: list, show <id>, add, edit <id>, delete <id>, save, exit\n");

    while (1) {
        printf("> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        cmd[strcspn(cmd, "\n")] = '\0';

        char *token = strtok(cmd, " ");
        if (!token) continue;
        if (strcmp(token, "list") == 0) {
            list_flights(root);
        } else if (strcmp(token, "show") == 0) {
            char *id = strtok(NULL, " ");
            if (id) show_flight(root, id);
            else printf("Uso: show <id>\n");
        } else if (strcmp(token, "add") == 0) {
            add_flight(root);
        } else if (strcmp(token, "edit") == 0) {
            char *id = strtok(NULL, " ");
            if (id) edit_flight(root, id);
            else printf("Uso: edit <id>\n");
        } else if (strcmp(token, "delete") == 0) {
            char *id = strtok(NULL, " ");
            if (id) delete_flight(root, id);
            else printf("Uso: delete <id>\n");
        } else if (strcmp(token, "save") == 0) {
            if (save_json(DATA_FILE, root) == 0)
                printf("Guardado en %s\n", DATA_FILE);
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("Comando no reconocido.\n");
        }
    }

    cJSON_Delete(root);
    return 0;
}

cJSON *load_json(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    cJSON *json = cJSON_Parse(data);
    free(data);
    return json;
}

int save_json(const char *filename, cJSON *json) {
    char *str = cJSON_Print(json);
    if (!str) return -1;
    FILE *f = fopen(filename, "w");
    if (!f) { free(str); return -1; }
    fprintf(f, "%s", str);
    fclose(f);
    free(str);
    return 0;
}

void list_flights(cJSON *array) {
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        printf("- %s\n", cJSON_GetObjectItem(item, "id")->valuestring);
    }
}

void show_flight(cJSON *array, const char *id) {
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (strcmp(cJSON_GetObjectItem(item, "id")->valuestring, id) == 0) {
            char *s = cJSON_Print(item);
            printf("%s\n", s);
            free(s);
            return;
        }
    }
    printf("Vuelo %s no encontrado.\n", id);
}

void add_flight(cJSON *array) {
    cJSON *item = cJSON_CreateObject();
    char *id = prompt("ID: ");
    cJSON_AddStringToObject(item, "id", id);
    free(id);
    char *gate = prompt("Puerta: ");
    cJSON_AddStringToObject(item, "gate", gate);
    free(gate);
    char *dep = prompt("Salida: ");
    cJSON_AddStringToObject(item, "departure", dep);
    free(dep);
    char *notes = prompt("Observaciones: ");
    cJSON_AddStringToObject(item, "notes", notes);
    free(notes);
    char *status = prompt("Estado: ");
    cJSON_AddStringToObject(item, "status", status);
    free(status);

    cJSON_AddItemToArray(array, item);
    printf("Vuelo agregado.\n");
}

void edit_flight(cJSON *array, const char *id) {
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (strcmp(cJSON_GetObjectItem(item, "id")->valuestring, id) == 0) {
            char *gate = prompt("Nueva puerta (dejar en blanco para omitir): ");
            if (gate[0]) cJSON_ReplaceItemInObject(item, "gate", cJSON_CreateString(gate));
            free(gate);
            char *dep = prompt("Nueva salida (dejar en blanco para omitir): ");
            if (dep[0]) cJSON_ReplaceItemInObject(item, "departure", cJSON_CreateString(dep));
            free(dep);
            char *notes = prompt("Nuevas observaciones (dejar en blanco para omitir): ");
            if (notes[0]) cJSON_ReplaceItemInObject(item, "notes", cJSON_CreateString(notes));
            free(notes);
            char *status = prompt("Nuevo estado (dejar en blanco para omitir): ");
            if (status[0]) cJSON_ReplaceItemInObject(item, "status", cJSON_CreateString(status));
            free(status);
            printf("Vuelo %s actualizado.\n", id);
            return;
        }
    }
    printf("Vuelo %s no encontrado.\n", id);
}

void delete_flight(cJSON *array, const char *id) {
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (strcmp(cJSON_GetObjectItem(item, "id")->valuestring, id) == 0) {
            cJSON_DeleteItemFromArray(array, i);
            printf("Vuelo %s eliminado.\n", id);
            return;
        }
    }
    printf("Vuelo %s no encontrado.\n", id);
}

char *prompt(const char *msg) {
    printf("%s", msg);
    char buffer[BUFFER_SIZE];
    if (!fgets(buffer, sizeof(buffer), stdin)) return strdup("");
    buffer[strcspn(buffer, "\n")] = '\0';
    return strdup(buffer);
}