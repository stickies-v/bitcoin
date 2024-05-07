// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_WARNINGS_H
#define BITCOIN_NODE_WARNINGS_H

#include <sync.h>
#include <util/translation.h>

#include <map>
#include <string>
#include <vector>

class UniValue;

namespace node {
/**
 * @class Warnings
 * @brief Manages warning messages within a node.
 *
 * The Warnings class provides mechanisms to set, unset, and retrieve
 * warning messages identified by unique strings. Warning messages are
 * managed in a thread-safe manner using mutexes. This class is designed
 * to be non-copyable to ensure warnings are managed centrally.
 */
class Warnings
{
    mutable Mutex m_mutex;
    std::map<std::string, bilingual_str> m_warnings GUARDED_BY(m_mutex);
public:
    Warnings();
    //! Warnings should always be passed by reference, never copied.
    Warnings(const Warnings&) = delete;
    Warnings& operator=(const Warnings&) = delete;
    /**
     * @brief Set a warning message. If a warning with the specified `id`
     *        already exists, false is returned and the new warning is
     *        ignored. If `id` does not yet exist, the warning is set,
     *        the UI is updated, and true is returned.
     *
     * @param[in]   id  Unique identifier of the warning. Uniqueness is
     *                  enforced only by the user.
     * @param[in]   message Warning message to be shown.
     *
     * @returns true if the warning was indeed set (i.e. there is no
     *          existing warning with this `id`), otherwise false.
     */
    bool Set(const std::string& id, const bilingual_str& message) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /**
     * @brief Unset a warning message. If a warning with the specified
     *        `id` exists, it is unset, the UI is updated, and true is
     *        returned. Otherwise, no warning is unset and false is
     *        returned
     *
     * @param[in]   id  Unique identifier of the warning. Uniqueness is
     *                  enforced only by the user.
     *
     * @returns true if the warning was indeed unset (i.e. there is an
     *          existing warning with this `id`), otherwise false.
     */
    bool Unset(const std::string& id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Return potential problems detected by the node. */
    std::vector<bilingual_str> GetMessages() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

/**
 * RPC helper function that wraps warnings.GetMessages().
 *
 * Returns a UniValue::VSTR with the latest warning if use_deprecated is
 * set to true, or a UniValue::VARR with all warnings otherwise.
*/
UniValue GetWarningsForRpc(const Warnings& warnings, bool use_deprecated);
} // namespace node

#endif // BITCOIN_NODE_WARNINGS_H
