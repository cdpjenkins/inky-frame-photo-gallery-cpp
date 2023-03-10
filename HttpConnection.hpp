#ifndef INKY_FRAME_PHOTO_GALLERY_CPP_HTTPCONNECTION_HPP
#define INKY_FRAME_PHOTO_GALLERY_CPP_HTTPCONNECTION_HPP

#include <string>
#include <vector>

static const size_t BUFFER_CAPACITY = 16384;

const size_t IP_V4_MAX_LENGTH = 20;     // longer than needed but meh
const size_t PATH_MAX_LENGTH = 128;

class HttpConnection {

public:
    HttpConnection(const char *ip_address_param,
                   int port,
                   const char *path,
                   const char *filename);

    virtual ~HttpConnection();

    void http_result_received(httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
    err_t http_headers_received(httpc_state_t *connection, pbuf *hdr, u16_t hdr_len, u32_t content_len);
    err_t http_body_received(tcp_pcb *conn, pbuf *p, err_t err);
    void do_request();

private:
    char *ip_address;
    int port;
    char *path;
    FIL file_handle;
    char *filename;
    int bytes_downloaded = 0;

    // Not sure if volatile is enough or if there are more powerful synchronisation primatives that need to be used here
    volatile bool completed = false;
    size_t size = 0;
};

void http_result_callback(void *arg,
                          httpc_result_t httpc_result,
                          u32_t rx_content_len,
                          u32_t srv_res,
                          err_t err
);

err_t http_headers_callback(httpc_state_t *connection,
                            void *arg,
                            struct pbuf *hdr,
                            u16_t hdr_len,
                            u32_t content_len
);

err_t http_body_callback(void *arg,
                         struct altcp_pcb *conn,
                         struct pbuf *p,
                         err_t err);

#endif //INKY_FRAME_PHOTO_GALLERY_CPP_HTTPCONNECTION_HPP
