======
ginger
======

Template is here

.. sourcecode:: html

  <html>
    <head>
      <title>${title}</title>
    </head>
    <body>
      <p>List of values:</p>
      <ul>
        $for x in xs {{
          $if x.enable {{
            <li><a href="${x.url}">${x.value}</a></li>
          }}
        }}
      </ul>
    </body>
  </html>

Source code is here

.. sourcecode:: cpp

  #include "ginger.h"
  #include <iostream>
  #include <sstream>
  #include <vector>
  #include <map>
  #include <string>

  int main() {
      std::vector<std::map<std::string, ginger::object>> xs;
      xs.push_back({
          { "enable", true },
          { "url", "http://example.com" },
          { "value", "Example" },
      });
      xs.push_back({
          { "enable", false },
          { "url", "undefined" },
          { "value", "Test" },
      });
      xs.push_back({
          { "enable", true },
          { "url", "http://google.com" },
          { "value", "Google" },
      });

      std::map<std::string, ginger::object> t;
      t["title"] = "Sample Site";
      t["xs"] = xs;

      std::stringstream ss;
      ss << std::cin.rdbuf();
      try {
          ginger::parse(ss.str(), t);
      } catch (ginger::parse_error& error) {
          std::cerr << error.long_error() << std::endl;
      }
  }

Result

.. sourcecode:: html

  <html>
    <head>
      <title>Sample Site</title>
    </head>
    <body>
      <p>List of values:</p>
      <ul>
        
          
            <li><a href="http://example.com">Example</a></li>
          
        
          
        
          
            <li><a href="http://google.com">Google</a></li>
          
        
      </ul>
    </body>
  </html>

Requirements
============

Input
-----

- below expression should be valid

  .. sourcecode:: cpp

    auto first = std::begin(input);
    auto last = std::end(input);

- first, last should be ForwardIterator.


Dictionary
----------

- Below expressions should be valid

  .. sourcecode:: cpp

    std::string var;
    auto it = dic.find(var);
    it != dic.end();

- Type of it->second should be object.

Output
------

- Below expression should be valid

  .. sourcecode:: cpp

    // Output output; // default constructible is not required.
    // For any ForwardIterator first, last
    output.put(first, last);
    output.flush();

Reference
=========

object
------

Class ``object`` can construct from any value.

.. sourcecode:: cpp

  class object {
  public:
      object() = default;
      object(const object&) = default;
      object(object&&) = default;
      object& operator=(const object&) = default;
      object& operator=(object&&) = default;

      template<class T> object(T v);
      template<class T> void operator=(T v);
  };

parse
-----

.. sourcecode:: cpp

  template<class Input, class Dictionary>
  void parse(Input&& input, Dictionary&& t);
  template<class Input, class Dictionary, class Output>
  void parse(Input&& input, Dictionary&& t, Output&& out);

  template<class Dictionary>
  void parse(const char* input, Dictionary&& t);
  template<class Dictionary, class Output>
  void parse(const char* input, Dictionary&& t, Output&& out);

Template Syntax Specification
=============================

::

  <root> = <block>
  <block> = (<char> | <$comment> | <$for> | <$if> | <$variable> | $$ | ${{ | $}}) <block> | <eof>
  <char> = any character without '$'
  <$comment> = $#<comment-char>*
  <comment-char> = any character without '\n' and <eof>
  <$for> = $for <var-name> in <var> {{ <block> }}
  <$if> = $if <var> {{ <block> }} ($elseif <var> {{ <block> }})? ($else {{ <block> }})?
  <$variable> = ${<var>}
  <var> = <var-name>(.<var-name>)*
  <var-name> = <var-char>+
  <var-char> = any character without <whitespace>, '.', '{' or '}'
  <whitespace> = c <= 32 where c is character
