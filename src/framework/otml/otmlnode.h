/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef OTMLNODE_H
#define OTMLNODE_H

#include "declarations.h"
#include <framework/ui/uimanager.h>

class OTMLNode : public std::enable_shared_from_this<OTMLNode>
{
public:
    virtual ~OTMLNode() { }

    static OTMLNodePtr create(std::string tag = "", bool unique = false);
    static OTMLNodePtr create(std::string tag, std::string value);

    std::string tag() { return m_tag; }
    int size() { return m_children.size(); }
    std::string source() { return m_source; }
    std::string rawValue() { return m_value; }

    bool isUnique() { return m_unique; }
    bool isNull() { return m_null; }

    bool hasTag() { return !m_tag.empty(); }
    bool hasValue() { return !m_value.empty(); }
    bool hasChildren();
    bool hasChildAt(const std::string& childTag) { return !!get(childTag); }
    size_t getIndex() { return m_index; }

    void setTag(const std::string& tag);
    void setValue(const std::string& value) { m_value = value; }
    void setNull(bool null) { m_null = null; }
    void setUnique(bool unique) { m_unique = unique; }
    void setSource(const std::string& source) { m_source = source; }
    void setIndex(size_t index) { m_index = index; }

    void lockTag() {
        m_tagLocked = true;
    }

    OTMLNodePtr get(const std::string& childTag);
    OTMLNodePtr getIndex(int childIndex);

    OTMLNodePtr at(const std::string& childTag);

    void addChild(const OTMLNodePtr& newChild);
    bool removeChild(const OTMLNodePtr& oldChild);
    void copy(const OTMLNodePtr& node);
    void merge(const OTMLNodePtr& node);
    void clear();

    OTMLNodeList children();
    OTMLNodePtr clone();

    template<typename T = std::string>
    T value(bool raw = false);
    template<typename T = std::string>
    T valueAt(const std::string& childTag);
    template<typename T = std::string>
    T valueAtIndex(int childIndex);
    template<typename T = std::string>
    T valueAt(const std::string& childTag, const T& def);
    template<typename T = std::string>
    T valueAtIndex(int childIndex, const T& def);

    template<typename T>
    void write(const T& v);
    template<typename T>
    void writeAt(const std::string& childTag, const T& v);
    template<typename T>
    void writeIn(const T& v);

    virtual std::string emit();

    OTMLNodePtr asOTMLNode() { return shared_from_this(); }

protected:
    OTMLNode() : m_unique(false), m_null(false) { }

    std::unordered_map<std::string, std::vector<OTMLNodePtr>> m_children;
    std::string m_tag;
    std::string m_value;
    std::string m_source;
    size_t m_index = 0;
    bool m_unique;
    bool m_null;
    bool m_tagLocked = false;
};

#include "otmlexception.h"

template<>
inline std::string OTMLNode::value<std::string>(bool raw) {
    std::string value = m_value;
    if(stdext::starts_with(value, "\"") && stdext::ends_with(value, "\"")) {
        value = value.substr(1, value.length()-2);
        stdext::replace_all(value, "\\\\", "\\");
        stdext::replace_all(value, "\\\"", "\"");
        stdext::replace_all(value, "\\t",  "\t");
        stdext::replace_all(value, "\\n",  "\n");
        stdext::replace_all(value, "\\'",  "\'");
    }
    if (!raw) {
        if (stdext::starts_with(value, "$var-")) {
            return g_ui.getOTUIVar(value);
        }
    }
    return value;
}

template<typename T>
T OTMLNode::value(bool raw) {
    T ret;
    if (stdext::starts_with(m_value, "$var-")) {
        if (!stdext::cast(g_ui.getOTUIVar(m_value), ret))
            throw OTMLException(asOTMLNode(), stdext::format("failed to cast var value '%s' (var value '%s') to type '%s'", m_value, g_ui.getOTUIVar(m_value), stdext::demangle_type<T>()));
        return ret;
    }
    if (!stdext::cast(m_value, ret))
        throw OTMLException(asOTMLNode(), stdext::format("failed to cast node value '%s' to type '%s'", m_value, stdext::demangle_type<T>()));
    return ret;
}

template<typename T>
T OTMLNode::valueAt(const std::string& childTag) {
    OTMLNodePtr node = at(childTag);
    return node->value<T>();
}

template<typename T>
T OTMLNode::valueAt(const std::string& childTag, const T& def) {
    if(OTMLNodePtr node = get(childTag))
        if(!node->isNull())
            return node->value<T>();
    return def;
}

template<typename T>
T OTMLNode::valueAtIndex(int childIndex, const T& def) {
    if(OTMLNodePtr node = getIndex(childIndex))
        return node->value<T>();
    return def;
}

template<typename T>
void OTMLNode::write(const T& v) {
    m_value = stdext::safe_cast<std::string>(v);
}

template<typename T>
void OTMLNode::writeAt(const std::string& childTag, const T& v) {
    OTMLNodePtr child = OTMLNode::create(childTag);
    child->setUnique(true);
    child->write<T>(v);
    addChild(child);
}

template<typename T>
void OTMLNode::writeIn(const T& v) {
    OTMLNodePtr child = OTMLNode::create();
    child->write<T>(v);
    addChild(child);
}

#endif

