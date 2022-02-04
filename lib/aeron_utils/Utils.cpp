#include "Utils.h"

/**
 * Функция, возвращающая Callback для печати сообщений на экран
 *
 * @return Callback для печати сообщений на экран
 */
aeron::fragment_handler_t print_string_message()
{
    return [&](const aeron::AtomicBuffer& buffer, aeron::util::index_t offset, aeron::util::index_t length,
            const aeron::Header& header)
    {
        std::cout << "Message to stream " << header.streamId() << " from session " << header.sessionId();
        std::cout << "(" << length << "@" << offset << ") <<";
        std::cout << std::string(reinterpret_cast<const char*>(buffer.buffer()) + offset,
                static_cast<std::size_t>(length)) << ">>" << std::endl;
    };
}
