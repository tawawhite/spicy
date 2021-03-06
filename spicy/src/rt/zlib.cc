// Copyright (c) 2020 by the Zeek Project. See LICENSE for details.

#include <zlib.h>

#include <hilti/rt/types/bytes.h>

#include <spicy/rt/zlib.h>

using namespace spicy::rt;
using namespace spicy::rt::zlib;

struct detail::State {
    z_stream stream;
};

Stream::Stream() {
    _state = std::shared_ptr<detail::State>(new detail::State(), [](auto p) {
        inflateEnd(&p->stream);
        delete p; // NOLINT(cppcoreguidelines-owning-memory)
    });

    // "15" here means maximum compression.  "32" is a gross overload hack
    // that means "check it for whether it's a gzip file". Sheesh.
    if ( inflateInit2(&_state->stream, 15 + 32) != Z_OK ) {
        _state = nullptr;
        throw ZlibError("inflateInit2 failed");
    }
}

// Don't finish the stream here, it might be shared with other instances.
Stream::~Stream() = default;

hilti::rt::Bytes Stream::finish() { return hilti::rt::Bytes(); }

hilti::rt::Bytes Stream::decompress(const hilti::rt::stream::View& data) {
    if ( ! _state )
        // Not sure if this should throw an exception instead. However, an
        // old comment indicated that this may be expected behaviour at least
        // with the HTTP analyzer, so leaving it as returning just empty dasa
        // for now.
        return hilti::rt::Bytes();

    hilti::rt::Bytes decoded;

    for ( auto block = data.firstBlock(); block; block = data.nextBlock(block) ) {
        do {
            char buf[4096];
            _state->stream.next_in = const_cast<Bytef*>(block->start);
            _state->stream.avail_in = block->size;
            _state->stream.next_out = reinterpret_cast<unsigned char*>(buf);
            _state->stream.avail_out = sizeof(buf);
            int zip_status = inflate(&_state->stream, Z_SYNC_FLUSH);

            if ( zip_status != Z_STREAM_END && zip_status != Z_OK && zip_status != Z_BUF_ERROR ) {
                _state = nullptr;
                throw ZlibError("inflate failed");
            }

            if ( auto len = (sizeof(buf) - _state->stream.avail_out) )
                decoded.append(hilti::rt::Bytes(buf, static_cast<int>(len)));

            if ( zip_status == Z_STREAM_END ) {
                finish();
                break;
            }

        } while ( _state->stream.avail_out == 0 );
    }

    return decoded;
}

hilti::rt::Bytes Stream::decompress(const hilti::rt::Bytes& data) {
    if ( ! _state )
        // Not sure if this should throw an exception instead. However, an
        // old comment indicated that this may be expected behaviour at least
        // with the HTTP analyzer, so leaving it as returning just empty dasa
        // for now.
        return hilti::rt::Bytes();

    hilti::rt::Bytes decoded;

    do {
        char buf[4096];
        _state->stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.data()));
        _state->stream.avail_in = data.size();
        _state->stream.next_out = reinterpret_cast<unsigned char*>(buf);
        _state->stream.avail_out = sizeof(buf);
        int zip_status = inflate(&_state->stream, Z_SYNC_FLUSH);

        if ( zip_status != Z_STREAM_END && zip_status != Z_OK && zip_status != Z_BUF_ERROR ) {
            _state = nullptr;
            throw ZlibError("inflate failed");
        }

        if ( auto len = (sizeof(buf) - _state->stream.avail_out) )
            decoded.append(hilti::rt::Bytes(buf, static_cast<int>(len)));

        if ( zip_status == Z_STREAM_END ) {
            finish();
            break;
        }

    } while ( _state->stream.avail_out == 0 );

    return decoded;
}
