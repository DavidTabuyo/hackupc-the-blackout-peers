#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <microhttpd.h>
#include "uthash.h"

#define PORT 8888

struct entry {
    char *id;
    char *gate;
    char *departure;
    char *notes;
    char *status;
    UT_hash_handle hh;
};

static struct entry *entries = NULL;

void add_entry(const char *id, const char *gate, const char *departure,
               const char *notes, const char *status) {
    struct entry *e = malloc(sizeof(struct entry));
    e->id = strdup(id);
    e->gate = strdup(gate);
    e->departure = strdup(departure);
    e->notes = strdup(notes);
    e->status = strdup(status);
    HASH_ADD_KEYPTR(hh, entries, e->id, strlen(e->id), e);
}

static enum MHD_Result send_notfound(struct MHD_Connection *connection, const char *id) {
    char page[1024];
    if (id) {
        snprintf(page, sizeof(page),
            "<!DOCTYPE html>"
            "<html lang=\"es\">"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>No Encontrado</title>"
            "<style>@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
            "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
            ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%;text-align:center;}"
            "h1{margin:0;color:#c62828;font-weight:500;}p{color:#555;margin:1rem 0;}"
            "button{margin-top:1rem;padding:.75rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%%;}"
            "button:hover{background:#0053cc;}"
            "</style></head><body><div class=\"card\">"
            "<h1>Vuelo no encontrado</h1>"
            "<p>ID: <strong>%s</strong></p>"
            "<button onclick=\"location.href='/'\">Inicio</button>"
            "</div></body></html>", id);
    } else {
        snprintf(page, sizeof(page),
            "<!DOCTYPE html>"
            "<html lang=\"es\">"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>No Encontrado</title>"
            "<style>@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
            "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
            ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%;text-align:center;}"
            "h1{margin:0;color:#c62828;font-weight:500;}"
            "button{margin-top:1rem;padding:.75rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%%;}"
            "button:hover{background:#0053cc;}"
            "</style></head><body><div class=\"card\">"
            "<h1>Vuelo no encontrado</h1>"
            "<button onclick=\"location.href='/'\">Inicio</button>"
            "</div></body></html>");
    }
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), page, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "text/html; charset=UTF-8");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return (enum MHD_Result)ret;
}

static enum MHD_Result answer_to_connection(void *cls,
                                            struct MHD_Connection *connection,
                                            const char *url,
                                            const char *method,
                                            const char *version,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls) {
    struct MHD_Response *response;
    int ret;
    if (0 != strcmp(method, "GET"))
        return MHD_NO;

    if (0 == strcmp(url, "/")) {
        const char *page =
            "<!DOCTYPE html>"
            "<html lang=\"es\">"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>Buscar Vuelo</title>"
            "<style>"
            "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
            "*, *::before, *::after { box-sizing: border-box; }"
            "body{margin:0;padding:0;display:flex;justify-content:center;align-items:center;height:100vh;background:#f0f2f5;font-family:'Roboto',sans-serif;}"
            ".card{background:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:360px;}"
            "h1{margin-bottom:1rem;font-weight:500;color:#333;}"
            "input{width:100%;padding:.75rem;border:1px solid #ccc;border-radius:4px 4px 0 0;border-bottom:none;font-size:1rem;display:block;}"
            "button{width:100%;padding:.75rem;border:none;border-radius:0 0 4px 4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;display:block;}"
            "button:hover{background:#0053cc;}"
            "</style>"
            "</head><body>"
            "<div class=\"card\">"
            "<h1>Buscar Vuelo</h1>"
            "<form id=\"flightForm\">"
            "<input type=\"text\" id=\"idInput\" name=\"id\" placeholder=\"ID del vuelo\" required autofocus>"
            "<button type=\"submit\">Ver Detalles</button>"
            "</form></div>"
            "<script>"
            "document.getElementById('flightForm').addEventListener('submit',function(e){e.preventDefault();var id=document.getElementById('idInput').value;window.location='/' + encodeURIComponent(id);});"
            "</script></body></html>";
        response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/html; charset=UTF-8");
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return (enum MHD_Result)ret;
    }

    if (url[1] != '\0') {
        const char *id = url + 1;
        struct entry *e;
        HASH_FIND_STR(entries, id, e);
        if (e) {
            const char *bg="#f0f2f5";
            const char *color="#888";
            if (strcmp(e->status,"A tiempo")==0){bg="#e6ffed";color="#2e7d32";}
            else if(strcmp(e->status,"Retrasado")==0){bg="#fff4e5";color="#e65100";}
            else if(strcmp(e->status,"Cerrado")==0){bg="#ffe5e5";color="#c62828";}
            char page[2048];
            snprintf(page, sizeof(page),
                     "<!DOCTYPE html>"
                     "<html lang=\"es\">"
                     "<head>"
                     "<meta charset=\"UTF-8\">"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                     "<title>Vuelo %s</title>"
                     "<style>@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap');"
                     "body{margin:0;padding:2rem;background:#f0f2f5;font-family:'Roboto',sans-serif;display:flex;justify-content:center;}"
                     ".card{background:#fff;padding:1.5rem;border-radius:8px;box-shadow:0 4px 16px rgba(0,0,0,0.1);max-width:400px;width:100%;}"
                     "h1{margin:0 0 1rem;font-weight:500;color:#333;}"
                     "p{margin:.5rem 0;color:#555;}"
                     ".status{display:inline-block;padding:.25rem .5rem;border-radius:4px;background:%s;color:%s;font-weight:500;margin-top:.5rem;}"
                     "button{margin-top:1.5rem;padding:.75rem 1rem;border:none;border-radius:4px;background:#0069ff;color:#fff;font-size:1rem;cursor:pointer;width:100%%;}"
                     "button:hover{background:#0053cc;}"
                     "</style>"
                     "</head><body>"
                     "<div class=\"card\">"
                     "<h1>Vuelo %s</h1>"
                     "<p><strong>Puerta:</strong> %s</p>"
                     "<p><strong>Salida:</strong> %s</p>"
                     "<p><strong>Observaciones:</strong> %s</p>"
                     "<p><span class=\"status\">%s</span></p>"
                     "<button onclick=\"location.href='/'\">Inicio</button>"
                     "</div></body></html>",
                     e->id, bg, color, e->id, e->gate, e->departure, e->notes, e->status);
            response = MHD_create_response_from_buffer(strlen(page), page, MHD_RESPMEM_MUST_COPY);
            MHD_add_response_header(response, "Content-Type", "text/html; charset=UTF-8");
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return (enum MHD_Result)ret;
        } else {
            return send_notfound(connection, id);
        }
    }

    return send_notfound(connection, NULL);
}

int main() {
    add_entry("IB1234", "Puerta A5", "08:30", "Embarque abierto", "A tiempo");
    add_entry("BA5678", "Puerta B12", "12:45", "Retraso 15 min", "Retrasado");
    add_entry("AF9012", "Puerta C3", "16:00", "Embarque cerrado", "Cerrado");
    add_entry("LH3456", "Puerta D7", "20:15", "Puerta cambiada a D9", "Retrasado");

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
    if (!daemon) { fprintf(stderr, "Error iniciando servidor.\n"); return 1; }
    printf("Servidor en http://localhost:%d/\n", PORT);
    getchar();
    MHD_stop_daemon(daemon);
    return 0;
}
