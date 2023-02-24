#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "pico/cyw43_arch.h"
#include "libraries/inky_frame/inky_frame.hpp"

#include <iostream>

#include "HttpConnection.hpp"

void http_result_callback(void *arg,
                          httpc_result_t httpc_result,
                          u32_t rx_content_len,
                          u32_t srv_res,
                          err_t err) {
    HttpConnection *this_request = reinterpret_cast<HttpConnection*>(arg);

    this_request->http_result_received(httpc_result, rx_content_len, srv_res, err);
}

err_t http_headers_callback(httpc_state_t *connection,
                            void *arg,
                            struct pbuf *hdr,
                            u16_t hdr_len,
                            u32_t content_len) {
    HttpConnection *this_request = reinterpret_cast<HttpConnection*>(arg);
    return this_request->http_headers_received(connection, hdr, hdr_len, content_len);
}

err_t http_body_callback(void *arg, struct tcp_pcb *conn, struct pbuf *p, err_t err) {
    HttpConnection *this_request = reinterpret_cast<HttpConnection*>(arg);
    return this_request->http_body_received(conn, p, err);
}

void HttpConnection::http_result_received(httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    f_close(&file_handle);

    completed = true;
}

err_t HttpConnection::http_headers_received(httpc_state_t *connection, pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    return ERR_OK;
}

err_t HttpConnection::http_body_received(tcp_pcb *conn, pbuf *p, err_t err) {
    u16_t size = p->tot_len;

    if (size >= BUFFER_CAPACITY) {
        std::cout << "Oh oh, we've exhausted the size of the buffer!" << std::endl;
        std::cout << "size: " << size << " BUFFER_CAPACITY: " << BUFFER_CAPACITY << std::endl;
        return ERR_MEM;
    }

    char buffer[BUFFER_CAPACITY];
    u16_t bytes_copied = pbuf_copy_partial(p, buffer, size, 0);
    buffer[bytes_copied] = '\0';

    UINT bw;
    FRESULT res = f_write(&file_handle, (const void*)buffer, bytes_copied, &bw);
    if (res || bw < bytes_copied) return ERR_ABRT; /* error or disk full */

    return ERR_OK;
}

void HttpConnection::do_request() {
    cyw43_arch_lwip_begin();
    std::cout << "Doing HTTP request... ";
    completed = false;
    httpc_connection_t settings;
    settings.result_fn = http_result_callback;
    settings.headers_done_fn = http_headers_callback;

    ip_addr_t server_addr;
    ip4_addr_set_u32(&server_addr, ipaddr_addr(ip_address));

    err_t err = httpc_get_file(&server_addr,
                               port,
                               path,
                               &settings,
                               http_body_callback,
                               this,
                               nullptr);
    std::cout << "Called httpc_get_file(), returned " << (int)err << std::endl;
    cyw43_arch_lwip_end();

    bool http_thang_done = false;
    while (!http_thang_done) {
        cyw43_arch_lwip_begin();
        if (completed) {
            http_thang_done = true;
        }
        cyw43_arch_lwip_end();
    }
}

HttpConnection::HttpConnection(const char *ip_address_param,
                               int port,
                               const char *path,
                               const char *filename)
        : port(port),
          file_handle(file_handle)
{
    // I'm not quite clear why we need to do all this. I'd rather just use std::strings but, for some reason, the HTTP
    // call is not sent if we do use strings at this point. Or if we use define ip_address and path as char arrays in
    // this class. Or if we don't explicitly add the '\0' to the end of the string (even though the length is
    // significantly shorter than the max length...)
    //
    // So I reckon something screwy is going on with caches or interrupts or memory getting trampled somehow or...
    // dunno. But it's seriously annoying and is making me think I should have just stuck with MicroPython!

    this->ip_address = new char[IP_V4_MAX_LENGTH];
    strlcpy(this->ip_address, ip_address_param, IP_V4_MAX_LENGTH - 1);
    this->ip_address[IP_V4_MAX_LENGTH - 1] = '\0';

    this->path = new char[PATH_MAX_LENGTH];
    strlcpy(this->path, path, PATH_MAX_LENGTH - 1);
    this->path[PATH_MAX_LENGTH - 1] = '\0';

    this->filename = new char[PATH_MAX_LENGTH];
    strlcpy(this->filename, filename, PATH_MAX_LENGTH - 1);
    this->filename[PATH_MAX_LENGTH - 1] = '\0';

    std::cout << "About to create file handle" << std::endl;
    std::cout << "About to open file" << std::endl;
    if (f_open(&file_handle, filename, FA_CREATE_ALWAYS | FA_WRITE)) {
        std::cout << "ERROR: failed to open " << filename << std::endl;
        return;

        // wish we could use exceptions...
    }
    std::cout << "File is open" << std::endl;
}

HttpConnection::~HttpConnection() {
    delete[] this->filename;
    delete[] this->path;
    delete[] this->ip_address;
}
