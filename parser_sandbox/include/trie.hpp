#ifndef OCCA_TRIE_HEADER2
#define OCCA_TRIE_HEADER2

#include <iostream>
#include <vector>
#include <map>

#include <limits.h>

namespace occa {
  class trieNode_t;
  typedef std::map<char, trieNode_t>    trieNodeMap_t;
  typedef trieNodeMap_t::iterator       trieNodeMapIterator;
  typedef trieNodeMap_t::const_iterator cTrieNodeMapIterator;

  //---[ Node ]-------------------------
  class trieNode_t {
  public:
    class result_t {
    public:
      int length;
      int valueIdx;

      result_t();
      result_t(const int length_, const int valueIdx);

      bool success() const {
        return (0 <= valueIdx);
      }
    };

    int valueIdx;
    trieNodeMap_t leaves;

    trieNode_t();
    trieNode_t(const int valueIdx_);

    void add(const char *c, const int valueIdx_);

    int nodeCount() const;
    int getValueIdx(const char *c) const;
    result_t get(const char *c, const int length) const;
    result_t get(const char *c, const int cIdx, const int length) const;
  };
  //====================================

  //---[ Trie ]-------------------------
  template <class TM>
  class trie_t {
  public:
    //---[ Result ]---------------------
    class result_t {
    public:
      trie_t<TM> *trie;
      int length;
      int valueIdx;

      result_t();
      result_t(const trie_t<TM> *trie_,
               const int length_ = 0,
               const int valueIdx_ = -1);

      inline bool success() const;
      inline const TM& value() const;
      inline TM& value();
    };
    //==================================

    trieNode_t root;
    TM defaultValue;
    std::vector<TM> values;

    bool isFrozen, autoFreeze;
    int nodeCount, baseNodeCount;
    char *chars;
    int *offsets, *leafCount;
    int *valueIndices;

    trie_t();

    void clear();
    bool isEmpty() const;

    void add(const char *c, const TM &value = TM());
    void add(const std::string &s, const TM &value = TM());

    void freeze();
    int freeze(const trieNode_t &node, int offset);
    void defrost();

    result_t getFirst(const char *c, const int length = INT_MAX) const;
    result_t trieGetFirst(const char *c, const int length) const;
    inline result_t getFirst(const std::string &s) const {
      return getFirst(s.c_str(), (int) s.size());
    }

    result_t get(const char *c, const int length = INT_MAX) const;
    inline result_t get(const std::string &s) const {
      return get(s.c_str(), (int) s.size());
    }

    bool has(const char c) const;
    bool trieHas(const char c) const;

    bool has(const char *c) const;
    bool has(const char *c, const int size) const;
    inline bool has(const std::string &s) const {
      return has(s.c_str(), s.size());
    }

    void print();
  };
  //====================================
}

#include "trie.tpp"

#endif
