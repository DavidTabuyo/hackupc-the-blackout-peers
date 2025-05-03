#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>  // librería para parsear JSON

#define PORT 8888
#define DATA_FILE "flights.json"

typedef struct {
    char *id;
    char *gate;
    char *departure;
    char *notes;
    char *status;
} Flight;

static Flight *flights = NULL;
static size_t flight_count = 0;

void load_flights() {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) { fprintf(stderr, "No se pudo abrir %s\n", DATA_FILE); return; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(data);
    if (!cJSON_IsArray(root)) { fprintf(stderr, "%s no contiene un array JSON válido\n", DATA_FILE); free(data); return; }
    flight_count = cJSON_GetArraySize(root);
    flights = calloc(flight_count, sizeof(Flight));
    for (size_t i = 0; i < flight_count; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        flights[i].id = strdup(cJSON_GetObjectItem(item, "id")->valuestring);
        flights[i].gate = strdup(cJSON_GetObjectItem(item, "gate")->valuestring);
        flights[i].departure = strdup(cJSON_GetObjectItem(item, "departure")->valuestring);
        flights[i].notes = strdup(cJSON_GetObjectItem(item, "notes")->valuestring);
        flights[i].status = strdup(cJSON_GetObjectItem(item, "status")->valuestring);
    }
    cJSON_Delete(root);
    free(data);
}

int find_flight(const char *id) {
    for (size_t i = 0; i < flight_count; i++) {
        if (strcmp(flights[i].id, id) == 0) return (int)i;
    }
    return -1;
}

static enum MHD_Result send_notfound(struct MHD_Connection *conn, const char *id) {
    char page[1024];
    if (id) {
        snprintf(page, sizeof(page),
            "<!DOCTYPE html><html lang='es'><head>"
            "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>No Encontrado</title><style>"
            "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
            "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
            ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%;text-align:center;}"
            "h1{margin:0;color:#c62828;font-weight:500;}p{color:#555;margin:1rem 0;}"
            "button{margin-top:1rem;padding:.75rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%;}"
            "button:hover{background:#0053cc;}"
            "</style></head><body><div class='card'>"
            "<h1>Vuelo no encontrado</h1>"
            "<p>ID: <strong>%s</strong></p>"
            "<button onclick=\"location.href='/'\">Inicio</button>"
            "</div></body></html>", id);
    } else {
        snprintf(page, sizeof(page),
            "<!DOCTYPE html><html lang='es'><head>"
            "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>No Encontrado</title><style>"
            "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
            "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
            ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%;text-align:center;}"
            "h1{margin:0;color:#c62828;font-weight:500;}"
            "button{margin-top:1rem;padding:.75rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%;}"
            "button:hover{background:#0053cc;}"
            "</style></head><body><div class='card'>"
            "<h1>Vuelo no encontrado</h1>"
            "<button onclick=\"location.href='/'\">Inicio</button>"
            "</div></body></html>");
    }
    struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(page), page, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(resp, "Content-Type", "text/html; charset=UTF-8");
    MHD_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
    MHD_destroy_response(resp);
    return MHD_YES;
}

static enum MHD_Result answer_to_connection(void *cls,
                                            struct MHD_Connection *conn,
                                            const char *url,
                                            const char *method,
                                            const char *version,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls) {
    if (!(strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0))
        return MHD_NO;

    load_flights();  // Cargar los vuelos cada vez que haya una solicitud

    if (strcmp(url, "/") == 0) {
        const char *page =
            "<!DOCTYPE html><html lang='es'><head>"
            "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>Buscar Vuelo</title><style>"
            "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');*{box-sizing:border-box;}"
            "body{margin:0;padding:0;display:flex;justify-content:center;align-items:center;height:100vh;background:#f0f2f5;font-family:'Roboto',sans-serif;}"
            ".card{background:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:360px;}"
            "h1{margin-bottom:1rem;font-weight:500;color:#333;}input,button{width:100%;padding:.75rem;font-size:1rem;}"
            "input{border:1px solid #ccc;border-radius:4px 4px 0 0;border-bottom:none;}"
            "button{border:none;border-radius:0 0 4px 4px;background:#0069ff;color:#fff;cursor:pointer;}"
            "button:hover{background:#0053cc;}"
            "</style></head><body><div class='card'>"
            "<h1>Buscar Vuelo</h1><form id='f'><input type='text' id='id' placeholder='ID del vuelo' required autofocus>"
            "<button type='submit'>Ver Detalles</button></form></div>"
            "<script>document.getElementById('f').addEventListener('submit',function(e){e.preventDefault();location.href='/' + encodeURIComponent(document.getElementById('id').value);});</script>"
            "</body></html>";
        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(resp, "Content-Type", "text/html; charset=UTF-8");
        MHD_queue_response(conn, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    if (url[1]) {
        const char *id = url + 1;
        int idx = find_flight(id);
        if (idx < 0) return send_notfound(conn, id);
        Flight *f = &flights[idx];
        const char *bg = "#f0f2f5", *color = "#888";
        if (strcmp(f->status, "A tiempo") == 0) { bg = "#e6ffed"; color = "#2e7d32"; }
        else if (strcmp(f->status, "Retrasado") == 0) { bg = "#fff4e5"; color = "#e65100"; }
        else if (strcmp(f->status, "Cerrado") == 0)   { bg = "#ffe5e5"; color = "#c62828"; }
        char page[2048];
        snprintf(page, sizeof(page),
                 "<!DOCTYPE html><html lang='es'><head>"
                 "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                 "<title>Vuelo %s</title><style>"
                 "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
                 "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
                 ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%%;}"
                 "h1{margin:0 0 1rem;font-weight:500;color:#333;}p{margin:.5rem 0;color:#555;}"
                 ".status{display:inline-block;padding:.25rem .5rem;border-radius:4px;background:%s;color:%s;font-weight:500;margin-top:.5rem;}"
                 "button{margin-top:1.5rem;padding:.75rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%%;}"
                 "button:hover{background:#0053cc;}"
                 "</style></head><body><div class='card'>"
                 "<h1>Vuelo %s</h1><p><strong>Puerta:</strong> %s</p>"
                 "<p><strong>Salida:</strong> %s</p><p><strong>Observaciones:</strong> %s</p>"
                 "<p><span class='status'>%s</span></p>"
                 "<button onclick=\"location.href='/'\">Inicio</button>"
                 "</div></body></html>",
                 f->id, bg, color, f->id, f->gate, f->departure, f->notes, f->status);
        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(page), page, MHD_RESPMEM_MUST_COPY);
        MHD_add_response_header(resp, "Content-Type", "text/html; charset=UTF-8");
        MHD_queue_response(conn, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    return send_notfound(conn, NULL);
}

int main() {
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT,
        NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
    if (!daemon) { fprintf(stderr, "Error iniciando servidor.\n"); return 1; }
    printf("Servidor en http://localhost:%d/\n", PORT);
    getchar();
    MHD_stop_daemon(daemon);
    return 0;
}
