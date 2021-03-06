// Copyright (c) 2020 by the Zeek Project. See LICENSE for details.

#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <hilti/ast/ctors/string.h>
#include <hilti/ast/expression.h>
#include <hilti/ast/expressions/ctor.h>
#include <hilti/ast/node.h>
#include <hilti/base/logger.h>
#include <hilti/base/result.h>
#include <hilti/base/util.h>
#include <hilti/compiler/coercion.h>
#include <hilti/global.h>

namespace hilti {

/** AST node captures an `&<tag>` attribute along with an optional argument. */
class Attribute : public NodeBase {
public:
    Attribute() = default;

    /**
     * Constructor for an attribute with no argument.
     *
     * @param tag name of the attribute, including the leading `&`
     * @param m meta data to associate with the node
     */
    Attribute(std::string tag, Meta m = Meta()) : NodeBase({node::none}, std::move(m)), _tag(std::move(tag)) {}

    /**
     * Constructor for an attribute coming with an argument. The argument
     * must be either an AST node representing an expression.
     *
     * @param tag name of the attribute, including the leading `&`
     * @param v node representing the argument to associate with the attribute; must be an expression
     * @param m meta data to associate with the node
     */
    Attribute(std::string tag, Node v, Meta m = Meta())
        : NodeBase({std::move(v)}, std::move(m)), _tag(std::move(tag)) {}

    /** Returns the name of the attribute, including the leading `&`. */
    auto tag() const { return _tag; }

    /** Returns true if an argument is associated with the attribute. */
    bool hasValue() const { return ! childs()[0].isA<node::None>(); }

    /**
     * Returns the attribute associated with the node.
     *
     * @exception `std::out_of_range` if the attribute does not have an argument
     */
    const Node& value() const { return childs().at(0); }

    /**
     * Returns the attributes argument as type `T`. `T` must be either an
     * `Expression`, or `std::string`. In the former case, the value must be
     * an AST expression node. In the latter case, the argument must be a
     * string constructor expression, and the returned value will be the
     * string it represents.
     *
     * @tparam T either `Expression` or `std::string`
     * @return the argument, or an error if the argument could not be interpreted as type `T`
     * @exception `std::out_of_range` if the attribute does not have an argument
     */
    template<typename T>
    Result<T> valueAs() const {
        if constexpr ( std::is_same<T, Expression>::value ) {
            if ( auto e = value().tryAs<Expression>() )
                return *e;

            return result::Error(hilti::util::fmt("value for attribute '%s' must be an expression", _tag));
        }

        if constexpr ( std::is_same<T, std::string>::value ) {
            if ( auto e = value().tryAs<expression::Ctor>() )
                if ( auto s = e->ctor().tryAs<ctor::String>() )
                    return s->value();
            return result::Error(hilti::util::fmt("value for attribute '%s' must be a string", _tag));
        }

        logger().internalError(hilti::util::fmt("unsupported attribute value type requested (%s)", typeid(T).name()));
    }

    /** Implements the `Node` interface. */
    auto properties() const { return node::Properties{{"tag", _tag}}; }

    bool operator==(const Attribute& other) const {
        if ( _tag != other._tag )
            return false;

        if ( auto x = valueAs<Expression>() ) {
            auto y = other.valueAs<Expression>();
            return y && *x == *y;
        }

        if ( auto x = valueAs<std::string>() ) {
            auto y = other.valueAs<std::string>();
            return y && *x == *y;
        }

        return false;
    }

    /**
     * Sets or replaces an attributes's associated argument.
     *
     * @param the node to assign as the attribute's argument
     * @return a new attribute with the value changed
     */
    static Attribute setValue(const Attribute& a, Node n) {
        auto x = Attribute(a);
        if ( a.childs().empty() )
            x.childs().emplace_back(std::move(n));
        else
            x.childs()[0] = std::move(n);

        return x;
    }

private:
    std::string _tag;
};

/**
 * Constructs an AST node from an attribute.
 */
inline Node to_node(Attribute i) { return Node(std::move(i)); }

/** AST node holding a set of `Attribute` nodes. */
class AttributeSet : public NodeBase {
public:
    /**
     * Constructs a set from from a vector of attributes.
     *
     * @param a vector to initialize attribute set from
     * @param m meta data to associate with the node
     */
    explicit AttributeSet(std::vector<Attribute> a, Meta m = Meta()) : NodeBase(nodes(std::move(a)), std::move(m)) {}

    /** Returns the set's attributes. */
    std::vector<Attribute> attributes() const { return childs<Attribute>(0, -1); }

    /**
     * Retrieves an attribute with a given name from the set. If multiple
     * attributes with that tag exist, it's undefined which is returned.
     *
     * @return attribute if found
     */
    std::optional<Attribute> find(std::string_view tag) const {
        for ( auto& a : attributes() )
            if ( a.tag() == tag )
                return a;

        return {};
    }

    /**
     * If an attribute of a given name exists and has an expression value,
     * the value is coerced to a specified type. If not, nothing is done.
     *
     * @return A successful return value if either the coercion succeeded
     * (then the result's value is true), or nothing was to be done (then the
     * result's value is false); a failures if a coercionk would have been
     * necessary, but failed.
     */
    Result<bool> coerceValueTo(const std::string& tag, const Type& dst) {
        for ( auto& n : childs() ) {
            auto a = n.as<Attribute>();
            if ( a.tag() != tag )
                continue;

            if ( auto e = a.valueAs<Expression>() ) {
                auto ne = coerceExpression(*e, dst);
                if ( ! ne.coerced )
                    return result::Error("cannot coerce attribute value");

                if ( ne.nexpr ) {
                    n = Attribute(tag, std::move(*ne.nexpr));
                    return true;
                }

                return false;
            }
        }

        return false;
    }

    /**
     * Returns true if there's an attribute with a given name in the set.
     *
     * @param true if found
     */
    bool has(std::string_view tag) const { return find(tag).has_value(); }

    /** Implements `Node` interface. */
    auto properties() const { return node::Properties{}; }

    bool operator==(const AttributeSet& other) const { return attributes() == other.attributes(); };

    /** Returns true if the set has at least one element. */
    operator bool() const { return ! childs().empty(); }

    /**
     * Returns a new attribute set that adds one element.
     *
     * @param s set to add to.
     * @param a element to add.
     * @return `s` with `a' added
     */
    static AttributeSet add(AttributeSet s, Attribute a) {
        s.addChild(std::move(a));
        return s;
    }

    /**
     * Returns a new attribute set that adds one element.
     *
     * @param s set to add to.
     * @param a element to add.
     * @return `s` with `a' added
     */
    static std::optional<AttributeSet> add(std::optional<AttributeSet> s, Attribute a) {
        if ( ! s )
            s = AttributeSet({}, a.meta());

        s->addChild(std::move(a));
        return s;
    }

    /**
     * Retrieves an attribute with a given name from a set, dealing correctly
     * with an unset optional set. If multiple attributes with that tag
     * exist, it's undefined which is returned.
     *
     * @param attrs set to inspect
     * @return attribute if found
     */
    static std::optional<Attribute> find(const std::optional<AttributeSet>& attrs, std::string_view tag) {
        if ( attrs )
            return attrs->find(tag);
        else
            return {};
    }

    /**
     * Returns true if there's an attribute with a given name in a set,
     * dealing correctly with an unset optional set.
     *
     * @param attrs set to inspect
     * @param true if found
     */
    static bool has(const std::optional<AttributeSet>& attrs, std::string_view tag) {
        if ( attrs )
            return attrs->has(tag);
        else
            return false;
    }

private:
    /**
     * Constructs an empty set.
     *
     * @note We make this private so that it's harder to create an empty set.
     * Usually we pass them around as optionals.
     *
     * @param m meta data to associate with the node
     */
    AttributeSet(Meta m = Meta()) : NodeBase({}, std::move(m)) {}
};

/**
 * Constructs an AST node from an attribute set.
 */
inline Node to_node(AttributeSet i) { return Node(std::move(i)); }
inline Node to_node(std::optional<AttributeSet>&& i) { return i ? to_node(*i) : node::none; }

} // namespace hilti
