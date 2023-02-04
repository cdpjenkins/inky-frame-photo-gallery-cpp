//
// Created by Chris Jenkins on 04/02/2023.
//

#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "pico/cyw43_arch.h"
#include "libraries/inky_frame/inky_frame.hpp"
#include <iostream>
#include "HttpRequest.hpp"

bool global_http_thang_done = false;

HttpRequest global_http_request;

void do_http_request() {

    cyw43_arch_lwip_begin();
    std::cout << "Doing HTTP request... ";
    global_http_thang_done = false;
    httpc_connection_t settings;
    settings.result_fn = http_result_callback;
    settings.headers_done_fn = http_headers;

    ip_addr_t server_addr;
    ip4_addr_set_u32(&server_addr, ipaddr_addr("192.168.1.51"));

    err_t err = httpc_get_file(&server_addr,
                              8000,
                              "/list.txt",
                              &settings,
                              http_body,
                              &global_http_request,
                              nullptr);
    std::cout << "Called httpc_get_file(), returned " << (int)err << std::endl;
    cyw43_arch_lwip_end();

    bool http_thang_done = false;
    while (!http_thang_done) {
      cyw43_arch_lwip_begin();
      if (global_http_thang_done) {
        http_thang_done = true;
      }
      cyw43_arch_lwip_end();
    }
}

void http_result_callback(void *arg,
                          httpc_result_t httpc_result,
                          u32_t rx_content_len,
                          u32_t srv_res,
                          err_t err) {

    HttpRequest *this_request = reinterpret_cast<HttpRequest*>(arg);

    this_request->http_result_received(httpc_result, rx_content_len, srv_res, err);
}

err_t http_headers(httpc_state_t *connection,
                   void *arg,
                   struct pbuf *hdr,
                   u16_t hdr_len,
                   u32_t content_len) {
    std::cout << "Headers received" << std::endl;
    std::cout << "Content length: " << content_len << std::endl;

    if (arg != & global_http_request) {
        std::cout << "Uh oh!!!1" << std::endl;

        std::cout << arg << std::endl;
    }

    return ERR_OK;
}

err_t http_body(void *arg, struct tcp_pcb *conn, struct pbuf *p, err_t err) {
    char myBuff[4096];

    std::cout << "Body:" << std::endl;
    u16_t bytes_copied = pbuf_copy_partial(p, myBuff, p->tot_len, 0);
    std::cout << bytes_copied << " bytes copied" << std::endl;
    myBuff[bytes_copied] = 0;
    std::cout << myBuff << std::endl;

    if (arg != & global_http_request) {
        std::cout << "Uh oh!!!1" << std::endl;

        std::cout << arg << std::endl;
    }

    return ERR_OK;
}

void HttpRequest::http_result_received(httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    std::cout << "Transfer complete" << std::endl;
    std::cout << "Local result: " << httpc_result << std::endl;
    std::cout << "HTTP result: " << srv_res << std::endl;

    global_http_thang_done = true;
}
