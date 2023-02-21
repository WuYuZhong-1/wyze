#ifndef _WYZE_WEBSOCKET_SESSION_H_
#define _WYZE_WEBSOCKET_SESSION_H_

#include "http_session.h"


namespace wyze {
namespace http {

#pragma pack(1)
struct WSFrameHead {
    enum OPCODE {
        /// 数据分片帧
        CONTINUE = 0,
        /// 文本帧
        TEXT_FRAME = 1,
        /// 二进制帧
        BIN_FRAME = 2,
        /// 断开连接
        CLOSE = 8,
        /// PING
        PING = 0X9,
        /// PONG
        PONG = 0XA
    };
    

};
#pragma pack()

}
}

#endif // !_WYZE_WEBSOCKET_SESSION_H_