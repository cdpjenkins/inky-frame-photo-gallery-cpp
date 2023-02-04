#ifndef INKY_FRAME_PHOTO_GALLERY_CPP_HTTPREQUEST_HPP
#define INKY_FRAME_PHOTO_GALLERY_CPP_HTTPREQUEST_HPP


class HttpRequest {

};


void do_http_request();

void http_result(void *arg,
                 httpc_result_t httpc_result,
                 u32_t rx_content_len,
                 u32_t srv_res,
                 err_t err
);

err_t http_headers(httpc_state_t *connection,
                   void *arg,
                   struct pbuf *hdr,
                   u16_t hdr_len,
                   u32_t content_len
);

err_t http_body(void *arg,
                struct altcp_pcb *conn,
                struct pbuf *p,
                err_t err);




#endif //INKY_FRAME_PHOTO_GALLERY_CPP_HTTPREQUEST_HPP
