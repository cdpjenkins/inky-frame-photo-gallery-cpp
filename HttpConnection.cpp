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
    std::cout << "Transfer complete" << std::endl;
    std::cout << "Local result: " << httpc_result << std::endl;
    std::cout << "HTTP result: " << srv_res << std::endl;

    completed = true;
}

err_t HttpConnection::http_headers_received(httpc_state_t *connection, pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    std::cout << "Headers received" << std::endl;
    std::cout << "Content length: " << content_len << std::endl;

    return ERR_OK;
}

err_t HttpConnection::http_body_received(tcp_pcb *conn, pbuf *p, err_t err) {

    size_t new_size = size + p->tot_len;
    if (new_size >= sizeof(buffer)) {
        std::cout << "Oh oh, we've exhausted the size of the buffer!" << std::endl;
        return ERR_MEM;
    }


    u16_t bytes_copied = pbuf_copy_partial(p, &buffer[size], p->tot_len, 0);
    size = new_size;
    buffer[new_size] = 0;
    std::cout << bytes_copied << " bytes copied" << std::endl;
    std::cout << "Size is now: " << size << std::endl;

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
    ip4_addr_set_u32(&server_addr, ipaddr_addr("192.168.1.51"));

    err_t err = httpc_get_file(&server_addr,
                               8000,
                               "/list.txt",
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

char *HttpConnection::get_content() {
    return buffer;
}
