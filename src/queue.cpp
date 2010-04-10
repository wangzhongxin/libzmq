/*
    Copyright (c) 2007-2010 iMatix Corporation

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../include/zmq.h"

#include "queue.hpp"
#include "socket_base.hpp"
#include "err.hpp"

int zmq::queue (class socket_base_t *insocket_,
        class socket_base_t *outsocket_)
{
    zmq_msg_t request_msg;
    int rc = zmq_msg_init (&request_msg);
    errno_assert (rc == 0);
    bool has_request = false;

    zmq_msg_t response_msg;
    rc = zmq_msg_init (&response_msg);
    errno_assert (rc == 0);
    bool has_response = false;

    zmq_pollitem_t items [2];
    items [0].socket = insocket_;
    items [0].fd = 0;
    items [0].events = ZMQ_POLLIN;
    items [0].revents = 0;
    items [1].socket = outsocket_;
    items [1].fd = 0;
    items [1].events = ZMQ_POLLIN;
    items [1].revents = 0;

    while (true) {
        rc = zmq_poll (&items [0], 2, -1);
        errno_assert (rc > 0);

        //  The algorithm below asumes ratio of request and replies processed
        //  under full load to be 1:1. The alternative would be to process
        //  replies first, handle request only when there are no more replies.

        //  Receive a new request.
        if (items [0].revents & ZMQ_POLLIN) {
            zmq_assert (!has_request);
            rc = insocket_->recv (&request_msg, ZMQ_NOBLOCK);
            errno_assert (rc == 0);
            items [0].events &= ~ZMQ_POLLIN;
            items [1].events |= ZMQ_POLLOUT;
            has_request = true;
        }

        //  Send the request further.
        if (items [1].revents & ZMQ_POLLOUT) {
            zmq_assert (has_request);
            rc = outsocket_->send (&request_msg, ZMQ_NOBLOCK);
            errno_assert (rc == 0);
            items [0].events |= ZMQ_POLLIN;
            items [1].events &= ~ZMQ_POLLOUT;
            has_request = false;
        }

        //  Get a new reply.
        if (items [1].revents & ZMQ_POLLIN) {
            zmq_assert (!has_response);
            rc = outsocket_->recv (&response_msg, ZMQ_NOBLOCK);
            errno_assert (rc == 0);
            items [0].events |= ZMQ_POLLOUT;
            items [1].events &= ~ZMQ_POLLIN;
            has_response = true;
        }

        //  Send the reply further.
        if (items [0].revents & ZMQ_POLLOUT) {\
            zmq_assert (has_response);
            rc = insocket_->send (&response_msg, ZMQ_NOBLOCK);
            errno_assert (rc == 0);
            items [0].events &= ~ZMQ_POLLOUT;
            items [1].events |= ZMQ_POLLIN;
            has_response = false;
        }
    }

    return 0;
}

