#ifndef GINGER_H_INCLUDED
#define GINGER_H_INCLUDED

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <map>
#include <vector>
#include <cassert>

namespace ginger {

class object;
class iterator {
    struct holder;
    template<class T> struct holder_impl;

    std::shared_ptr<holder> holder_;
public:
    template<class T> iterator(T it);

    void operator++();
    object operator*();
    bool operator!=(iterator it) const;
};

class object {
    struct holder {
        virtual ~holder() { }
        virtual bool cond() const = 0;
        virtual iterator xbegin() = 0;
        virtual iterator xend() = 0;
        virtual std::string str() const = 0;
        virtual object get(std::string name) = 0;
    };

    template<class T>
    struct holder_impl : holder {
        T obj;
        holder_impl(T obj) : obj(std::move(obj)) { }

        template<class U, int = sizeof(std::declval<U>() ? true : false)>
        bool cond_(int) const {
            return static_cast<bool>(obj);
        }
        template<class>
        bool cond_(...) const {
            throw __LINE__;
        }
        virtual bool cond() const override {
            return cond_<T>(0);
        }

        template<class U, std::size_t = sizeof(std::declval<U>().begin(), std::declval<U>().end())>
        iterator begin_(short) {
            return obj.begin();
        }
        template<class>
        iterator begin_(...) {
            throw __LINE__;
        }
        virtual iterator xbegin() override {
            return begin_<T>(0);
        };

        template<class U, std::size_t = sizeof(std::declval<U>().begin(), std::declval<U>().end())>
        iterator end_(short) {
            return obj.end();
        }
        template<class>
        iterator end_(...) {
            throw __LINE__;
        }
        virtual iterator xend() override {
            return end_<T>(0);
        }

        template<class U, int = sizeof(std::declval<std::stringstream&>() << std::declval<U>())>
        std::string str_(int) const {
            std::stringstream ss;
            ss << obj;
            return ss.str();
        }
        template<class U>
        std::string str_(...) const {
            throw __LINE__;
        }
        virtual std::string str() const override {
            return str_<T>(0);
        }

        template<class U, int = sizeof(std::declval<U>()[std::declval<std::string>()])>
        object get_(int, std::string name) {
            return obj[std::move(name)];
        }
        template<class U>
        object get_(long, std::string, ...) {
            throw __LINE__;
        }
        virtual object get(std::string name) override {
            return get_<T>(0, std::move(name));
        }
    };

    std::shared_ptr<holder> holder_;

public:
    object() = default;
    object(const object&) = default;
    object(object&&) = default;
    object& operator=(const object&) = default;
    object& operator=(object&&) = default;

    template<class T>
    object(T v) : holder_(new holder_impl<T>(std::move(v))) { }

    template<class T>
    void operator=(T v) { holder_.reset(new holder_impl<T>(std::move(v))); }

    explicit operator bool() const { return holder_->cond(); }
    iterator xbegin() { return holder_->xbegin(); }
    iterator xend() { return holder_->xend(); }
    std::string str() const { return holder_->str(); }
    object operator[](std::string name) { return holder_->get(std::move(name)); }
};

iterator begin(object& obj) {
    return obj.xbegin();
}
iterator end(object& obj) {
    return obj.xend();
}

struct iterator::holder {
    virtual ~holder() { }
    virtual void operator++() = 0;
    virtual object operator*() = 0;
    virtual bool operator!=(iterator it) const = 0;
};
template<class T>
struct iterator::holder_impl : iterator::holder {
    T it;
    holder_impl(T it) : it(std::move(it)) { }
    virtual void operator++() override {
        ++it;
    }
    virtual object operator*() override {
        return *it;
    }
    virtual bool operator!=(iterator it) const override {
        return this->it != std::static_pointer_cast<holder_impl<T>>(it.holder_)->it;
    }
};
template<class T>
iterator::iterator(T it)
    : holder_(new iterator::holder_impl<T>(std::move(it))) { }

void iterator::operator++() {
    holder_->operator++();
}
object iterator::operator*() {
    return holder_->operator*();
}
bool iterator::operator!=(iterator it) const {
    return holder_->operator!=(it);
}

typedef std::map<std::string, object> temple;

namespace internal {

typedef std::map<std::string, std::vector<object>> tmpl_context;

template<class Iterator>
class parser {
public:
    Iterator current_;
    Iterator next_;
    Iterator last_;

public:
    typedef std::pair<Iterator, Iterator> range_t;

    parser(Iterator first, Iterator last) : next_(first), last_(last) {
        assert(first != last);
        current_ = next_++;
    }

    void read() {
        if (current_ == last_)
            throw __LINE__;
        current_ = next_;
        if (next_ != last_)
            ++next_;
    }

    explicit operator bool() const {
        return current_ != last_;
    }
    char peek() const {
        if (current_ == last_)
            throw __LINE__;
        return *current_;
    }
    bool has_next() const {
        return next_ != last_;
    }
    char next() const {
        if (next_ == last_)
            throw __LINE__;
        return *next_;
    }

    Iterator save() const {
        return current_;
    }
    void load(Iterator context) {
        current_ = next_ = context;
        if (next_ != last_)
            ++next_;
    }

    template<class F>
    range_t read_while(F f) {
        if (current_ == last_)
            throw __LINE__;
        auto first = current_;
        while (f(peek()))
            read();
        return std::make_pair(first, current_);
    }
    template<class F>
    range_t read_while_or_eof(F f) {
        auto first = current_;
        while (current_ != last_ && f(*current_))
            read();
        return std::make_pair(first, current_);
    }

    void skip_whitespace() {
        read_while([](char c) { return c <= 32; });
    }
    range_t read_ident() {
        skip_whitespace();
        return read_while([](char c) { return c > 32 && c != '}'; });
    }
    std::string read_ident_str() {
        auto r = read_ident();
        return std::string(r.first, r.second);
    }
    range_t read_variable() {
        return read_while([](char c) { return c > 32 && c != '.' && c != '}'; });
    }
    std::string read_variable_str() {
        auto r = read_variable();
        return std::string(r.first, r.second);
    }

    void eat(char c) {
        if (peek() != c)
            throw __LINE__;
        read();
    }
    void eat(const char* p) {
        while (*p)
            eat(*p++);
    }
    void eat_with_whitespace(const char* p) {
        skip_whitespace();
        eat(p);
    }
    static bool equal(std::pair<Iterator, Iterator> p, const char* str) {
        while (p.first != p.second && *str)
            if (*p.first++ != *str++)
                return false;
        return p.first == p.second && not *str;
    }
};

template<class F, std::size_t N>
void output_string(F& out, const char (&s)[N]) {
    out.put(s, s + N);
}

template<class Iterator>
object get_variable(parser<Iterator>& p, temple& dic, tmpl_context& ctx, bool skip) {
    p.skip_whitespace();
    if (skip) {
        p.read_variable();
        while (p.peek() == '.') {
            p.read();
            p.read_variable();
        }
        return object();
    } else {
        std::string var = p.read_variable_str();
        auto it = ctx.find(var);
        object obj;
        if (it != ctx.end() && not it->second.empty()) {
            obj = it->second.back();
        } else {
            auto it2 = dic.find(var);
            if (it2 != dic.end()) {
                obj = dic[var];
            } else {
                throw __LINE__;
            }
        }
        while (p.peek() == '.') {
            p.read();
            std::string var = p.read_variable_str();
            obj = obj[var];
        }
        return obj;
    }
}

template<class Iterator, class F, class G>
void block(parser<Iterator>& p, temple& dic, tmpl_context& ctx, bool skip, F& out, G& err) {
    while (p) {
        auto r = p.read_while_or_eof([](char c) { return c != '}' && c != '$'; });
        if (not skip)
            out.put(r.first, r.second);
        if (!p)
            break;

        char c = p.peek();
        if (c == '}') {
            if (p.has_next() && p.next() == '}') {
                // end of block
                break;
            } else {
                p.read();
                if (not skip)
                    output_string(out, "}");
            }
        } else if (c == '$') {
            p.read();
            c = p.peek();
            if (c == '$') {
                // $$
                p.read();
                if (not skip)
                    output_string(out, "$");
            } else if (c == '#') {
                // $# comments
                p.read_while_or_eof([](char peek) {
                    return peek != '\n';
                });
            } else if (c == '{') {
                p.read();
                c = p.peek();
                if (c == '{') {
                    // ${{
                    p.read();
                    if (not skip)
                        output_string(out, "{{");
                } else {
                    // ${variable}
                    object obj = get_variable(p, dic, ctx, skip);
                    p.eat_with_whitespace("}");

                    if (not skip) {
                        std::string str = obj.str();
                        out.put(str.begin(), str.end());
                    }
                }
            } else if (c == '}') {
                p.read();
                c = p.peek();
                if (c == '}') {
                    // $}}
                    p.read();
                    if (not skip)
                        output_string(out, "}}");
                } else {
                    throw __LINE__;
                }
            } else {
                auto command = p.read_ident();
                if (p.equal(command, "for")) {
                    // $for x in xs {{ <block> }}
                    auto var1 = p.read_ident_str();
                    if (not p.equal(p.read_ident(), "in"))
                        throw __LINE__;
                    object obj = get_variable(p, dic, ctx, skip);
                    p.eat_with_whitespace("{{");

                    if (skip) {
                        block(p, dic, ctx, true, out, err);
                    } else {
                        auto context = p.save();
                        auto& vec = ctx[var1];
                        for (object v: obj) {
                            vec.push_back(v);
                            block(p, dic, ctx, skip, out, err);
                            vec.pop_back();
                            p.load(context);
                        }
                        block(p, dic, ctx, true, out, err);
                    }
                    p.eat("}}");
                } else if (p.equal(command, "if")) {
                    // $if x {{ <block> }}
                    // $elseif y {{ <block> }}
                    // $elseif z {{ <block> }}
                    // $else {{ <block> }}
                    object obj = get_variable(p, dic, ctx, skip);
                    p.eat_with_whitespace("{{");
                    bool run; // if `skip` is true, `run` is an unspecified value.
                    if (skip) {
                        block(p, dic, ctx, true, out, err);
                    } else {
                        run = static_cast<bool>(obj);
                        block(p, dic, ctx, not run, out, err);
                    }
                    p.eat("}}");
                    while (true) {
                        auto context = p.save();
                        p.skip_whitespace();
                        c = p.peek();
                        if (c == '$') {
                            p.read();
                            auto command = p.read_ident();
                            if (p.equal(command, "elseif")) {
                                object obj = get_variable(p, dic, ctx, skip);
                                p.eat_with_whitespace("{{");
                                if (skip || run) {
                                    block(p, dic, ctx, true, out, err);
                                } else {
                                    bool run_ = static_cast<bool>(obj);
                                    block(p, dic, ctx, not run_, out, err);
                                    if (run_)
                                        run = true;
                                }
                                p.eat("}}");
                            } else if (p.equal(command, "else")) {
                                p.eat_with_whitespace("{{");
                                block(p, dic, ctx, skip || not run, out, err);
                                p.eat("}}");
                                break;
                            } else {
                                p.load(context);
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                } else {
                    throw __LINE__;
                }
            }
        } else {
            assert(false && "must not go through.");
            throw __LINE__;
        }
    }
}

template<class IOS>
struct ios_type {
    IOS ios;
    ios_type(IOS ios) : ios(std::forward<IOS>(ios)) { }

    template<class Iterator>
    void put(Iterator first, Iterator last) {
        std::copy(first, last, std::ostreambuf_iterator<char>(ios));
    }
    void flush() {
        ios << std::flush;
    }

    ios_type(ios_type&&) = default;
    ios_type& operator=(ios_type&&) = default;

    ios_type(const ios_type&) = delete;
    ios_type& operator=(const ios_type&) = delete;
};

}

template<class IOS>
internal::ios_type<IOS> from_ios(IOS&& ios) {
    return internal::ios_type<IOS>(std::forward<IOS>(ios));
}

template<class F, class G>
static void parse(std::string input, temple& t, F out, G err) {
    internal::parser<std::string::iterator> p{input.begin(), input.end()};
    internal::tmpl_context ctx;
    internal::block(p, t, ctx, false, out, err);
    out.flush();
    err.flush();
}
template<class F>
static void parse(std::string input, temple& t, F out) {
    parse(input, t, std::move(out), from_ios(std::cerr));
}
static void parse(std::string input, temple& t) {
    parse(input, t, from_ios(std::cout), from_ios(std::cerr));
}

}

#endif // GINGER_H_INCLUDED
