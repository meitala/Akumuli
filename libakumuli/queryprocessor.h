/**
 * Copyright (c) 2015 Eugene Lazin <4lazin@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <chrono>
#include <memory>

#include "akumuli.h"
#include "stringpool.h"

namespace Akumuli {
namespace QP {

struct Node {

    enum NodeType {
        // Samplers
        RandomSampler,
        Resampler,
        // Joins
        JoinByTimestamp,
        // Filtering
        FilterById,
        // Testing
        Mock,
    };

    virtual ~Node() = default;
    //! Complete adding values
    virtual void complete() = 0;
    //! Process value
    virtual void put(aku_Timestamp ts, aku_ParamId id, double value) = 0;

    // Introspections

    //! Get node type
    virtual NodeType get_type() const = 0;
};


struct NodeException : std::runtime_error {
    Node::NodeType type_;
    NodeException(Node::NodeType type, const char* msg);
    Node::NodeType get_type() const;
};


struct NodeBuilder {
    //! Create random sampling node
    static std::shared_ptr<Node> make_random_sampler(std::string type,
                                                     size_t buffer_size, std::shared_ptr<Node> next,
                                                     aku_logger_cb_t logger);

    //! Create filtering node
    static std::shared_ptr<Node> make_filter_by_id(aku_ParamId id, std::shared_ptr<Node> next,
                                                   aku_logger_cb_t logger);

    //! Create filtering node
    static std::shared_ptr<Node> make_filter_by_id_list(std::vector<aku_ParamId> ids, std::shared_ptr<Node> next,
                                                        aku_logger_cb_t logger);
};


/** Query processor.
  * Should be built from textual representation (json at first).
  * Should be used by both sequencer and page to match parameters
  * and group them together.
  */
struct QueryProcessor {

    typedef StringTools::StringT StringT;
    typedef StringTools::TableT TableT;

    //! Lowerbound
    const aku_Timestamp                lowerbound;
    //! Upperbound
    const aku_Timestamp                upperbound;
    //! Scan direction
    const int                          direction;
    //! List of metrics of interest
    const std::vector<std::string>     metrics;
    //! Name to id mapping
    TableT                             namesofinterest;

    //! Root of the processing topology
    std::shared_ptr<Node>              root_node;

    /** Create new query processor.
      * @param root is a root of the processing topology
      * @param metrics is a list of metrics of interest
      * @param begin is a timestamp to begin from
      * @param end is a timestamp to end with
      *        (depending on a scan direction can be greater or smaller then lo)
      */
    QueryProcessor(std::shared_ptr<Node> root,
                   std::vector<std::string> metrics,
                   aku_Timestamp begin,
                   aku_Timestamp end);

    /** Match param_id. Return group id on success or
      * negative value on error.
      */
    int match(uint64_t param_id);
};

}}  // namespaces
