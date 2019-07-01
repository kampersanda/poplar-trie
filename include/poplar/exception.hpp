/**
 * MIT License
 *
 * Copyright (c) 2018–2019 Shunsuke Kanda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef POPLAR_TRIE_EXCEPTION_HPP
#define POPLAR_TRIE_EXCEPTION_HPP

#include <exception>

namespace poplar {

class exception : public std::exception {
  public:
    explicit exception(const char* msg) : msg_{msg} {}
    ~exception() throw() override = default;

    const char* what() const throw() override {
        return msg_;
    }

  private:
    const char* msg_;
};

#define POPLAR_TO_STR_(n) #n
#define POPLAR_TO_STR(n) POPLAR_TO_STR_(n)
#define POPLAR_THROW(msg) throw poplar::exception(__FILE__ ":" POPLAR_TO_STR(__LINE__) ":" msg)
#define POPLAR_THROW_IF(cond, msg) (void)((!(cond)) || (POPLAR_THROW(msg), 0))

}  // namespace poplar

#endif  // POPLAR_TRIE_EXCEPTION_HPP
