/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "router_core_private.h"
#include <qpid/dispatch/amqp.h>
#include <stdio.h>

//
// NOTE: If the in_delivery argument is NULL, the resulting out deliveries
//       shall be pre-settled.
//
typedef int (*qdr_forward_message_t) (qdr_core_t      *core,
                                      qdr_address_t   *addr,
                                      qd_message_t    *msg,
                                      qdr_delivery_t  *in_delivery,
                                      bool             exclude_inprocess,
                                      bool             control);

typedef void (*qdr_forward_attach_t) (qdr_core_t      *core,
                                      qdr_forwarder_t *forw,
                                      qdr_link_t      *link);

struct qdr_forwarder_t {
    qdr_forward_message_t forward_message;
    qdr_forward_attach_t  forward_attach;
    bool                  bypass_valid_origins;
};

//==================================================================================
// Built-in Forwarders
//==================================================================================


qdr_delivery_t *qdr_forward_new_delivery(qdr_delivery_t *peer, qdr_link_t *link, qd_message_t *msg)
{
    qdr_delivery_t *dlv = new_qdr_delivery_t();

    ZERO(dlv);
    dlv->link    = link;
    dlv->peer    = peer;
    dlv->msg     = qd_message_copy(msg);
    dlv->settled = !peer || peer->settled;

    if (peer->peer == 0)
        peer->peer = dlv;  // TODO - make this a back-list for multicast tracking

    return dlv;
}


int qdr_forward_multicast_CT(qdr_core_t      *core,
                             qdr_address_t   *addr,
                             qd_message_t    *msg,
                             qdr_delivery_t  *in_delivery,
                             bool             exclude_inprocess,
                             bool             control)
{
    bool bypass_valid_origins = addr->forwarder->bypass_valid_origins;
    int  fanout = 0;

    //
    // Forward to local subscribers
    //
    qdr_link_ref_t *link_ref = DEQ_HEAD(addr->rlinks);
    while (link_ref) {
        qdr_link_t     *out_link     = link_ref->link;
        qdr_delivery_t *out_delivery = qdr_forward_new_delivery(in_delivery, out_link, msg);
        DEQ_INSERT_TAIL(out_link->undelivered, out_delivery); // TODO - check locking on this list
        qdr_connection_activate_CT(core, out_link->conn);
        fanout++;
        link_ref = DEQ_NEXT(link_ref);
    }

    //
    // Forward to remote routers with subscribers using the appropriate
    // link for the traffic class: control or data
    //
    //
    // Get the mask bit associated with the ingress router for the message.
    // This will be compared against the "valid_origin" masks for each
    // candidate destination router.
    //
    int origin = -1;
    qd_field_iterator_t *ingress_iter = in_delivery ? in_delivery->origin : 0;

    if (ingress_iter && !bypass_valid_origins) {
        qd_address_iterator_reset_view(ingress_iter, ITER_VIEW_NODE_HASH);
        qdr_address_t *origin_addr;
        qd_hash_retrieve(core->addr_hash, ingress_iter, (void*) &origin_addr);
        if (origin_addr && DEQ_SIZE(origin_addr->rnodes) == 1) {
            qdr_router_ref_t *rref = DEQ_HEAD(origin_addr->rnodes);
            origin = rref->router->mask_bit;
        }
    } else
        origin = 0;

    //
    // Forward to the next-hops for remote destinations.
    //
    if (origin >= 0) {
        qdr_router_ref_t *dest_node_ref = DEQ_HEAD(addr->rnodes);
        qdr_link_t       *dest_link;
        qdr_node_t       *next_node;
        qd_bitmask_t     *link_set = qd_bitmask(0);

        //
        // Loop over the target nodes for this address.  Build a set of outgoing links
        // for which there are valid targets.  We do this to avoid sending more than one
        // message down a given link.  It's possible that there are multiple destinations
        // for this address that are all reachable over the same link.  In this case, we
        // will send only one copy of the message over the link and allow a downstream
        // router to fan the message out.
        //
        while (dest_node_ref) {
            if (dest_node_ref->router->next_hop)
                next_node = dest_node_ref->router->next_hop;
            else
                next_node = dest_node_ref->router;
            dest_link = control ? next_node->peer_control_link : next_node->peer_data_link;
            if (dest_link && qd_bitmask_value(dest_node_ref->router->valid_origins, origin))
                qd_bitmask_set_bit(link_set, dest_link->conn->mask_bit);
            dest_node_ref = DEQ_NEXT(dest_node_ref);
        }

        //
        // Send a copy of the message outbound on each identified link.
        //
        int link_bit;
        while (qd_bitmask_first_set(link_set, &link_bit)) {
            qd_bitmask_clear_bit(link_set, link_bit);
            dest_link = control ?
                core->control_links_by_mask_bit[link_bit] :
                core->data_links_by_mask_bit[link_bit];
            if (dest_link) {
                qdr_delivery_t *out_delivery = qdr_forward_new_delivery(in_delivery, dest_link, msg);
                DEQ_INSERT_TAIL(dest_link->undelivered, out_delivery); // TODO - check locking on this list
                fanout++;
                addr->deliveries_transit++;
                qdr_connection_activate_CT(core, dest_link->conn);
            }
        }

        qd_bitmask_free(link_set);
    }

    if (!exclude_inprocess) {
        //
        // Forward to in-process subscribers
        //
    }

    return fanout;
}


int qdr_forward_closest_CT(qdr_core_t      *core,
                           qdr_address_t   *addr,
                           qd_message_t    *msg,
                           qdr_delivery_t  *in_delivery,
                           bool             exclude_inprocess,
                           bool             control)
{
    return 0;
}


int qdr_forward_balanced_CT(qdr_core_t      *core,
                            qdr_address_t   *addr,
                            qd_message_t    *msg,
                            qdr_delivery_t  *in_delivery,
                            bool             exclude_inprocess,
                            bool             control)
{
    return 0;
}


void qdr_forward_link_balanced_CT(qdr_core_t      *core,
                                  qdr_forwarder_t *forw,
                                  qdr_link_t      *link)
{
}


//==================================================================================
// In-Thread API Functions
//==================================================================================

qdr_forwarder_t *qdr_new_forwarder(qdr_forward_message_t fm, qdr_forward_attach_t fa, bool bypass_valid_origins)
{
    qdr_forwarder_t *forw = NEW(qdr_forwarder_t);

    forw->forward_message      = fm;
    forw->forward_attach       = fa;
    forw->bypass_valid_origins = bypass_valid_origins;

    return forw;
}


void qdr_forwarder_setup_CT(qdr_core_t *core)
{
    //
    // Create message forwarders
    //
    core->forwarders[QD_SEMANTICS_MULTICAST_FLOOD]  = qdr_new_forwarder(qdr_forward_multicast_CT, 0, true);
    core->forwarders[QD_SEMANTICS_MULTICAST_ONCE]   = qdr_new_forwarder(qdr_forward_multicast_CT, 0, false);
    core->forwarders[QD_SEMANTICS_ANYCAST_CLOSEST]  = qdr_new_forwarder(qdr_forward_closest_CT,   0, false);
    core->forwarders[QD_SEMANTICS_ANYCAST_BALANCED] = qdr_new_forwarder(qdr_forward_balanced_CT,  0, false);

    //
    // Create link forwarders
    //
    core->forwarders[QD_SEMANTICS_LINK_BALANCED] = qdr_new_forwarder(0, qdr_forward_link_balanced_CT, false);
}


qdr_forwarder_t *qdr_forwarder_CT(qdr_core_t *core, qd_address_semantics_t semantics)
{
    if (semantics <= QD_SEMANTICS_LINK_BALANCED)
        return core->forwarders[semantics];
    return 0;
}


int qdr_forward_message_CT(qdr_core_t *core, qdr_address_t *addr, qd_message_t *msg, qdr_delivery_t *in_delivery,
                           bool exclude_inprocess, bool control)
{
    if (addr->forwarder)
        return addr->forwarder->forward_message(core, addr, msg, in_delivery, exclude_inprocess, control);

    // TODO - Deal with this delivery's disposition
    return 0;
}


void qdr_forward_attach_CT(qdr_core_t *core, qdr_forwarder_t *forwarder, qdr_link_t *in_link)
{
}

