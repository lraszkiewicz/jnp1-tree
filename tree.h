// Łukasz Raszkiewicz 371594

#ifndef TREE_H_
#define TREE_H_

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <tuple>

template <typename T>
class Tree {

  class Node {
   public:
    template <typename F, typename U>
    static U foldNode(std::shared_ptr<Node> node, F operation, U init) {
      if (!node)
        return init;
      else
        return operation(node->getValue(),
                         foldNode(node->_left, operation, init),
                         foldNode(node->_right, operation, init));
    }

    explicit Node(T value)
      : _value(value) {}

    Node(T value, std::shared_ptr<Node> left, std::shared_ptr<Node> right)
      : _value(value), _left(left), _right(right) {}

    T getValue() {
      while (!transformers.empty()) {
        _value = transformers.front()(_value);
        transformers.pop_front();
      }
      return _value;
    }

    void addTransformer(std::function<T(T)> transformer) {
      transformers.push_back(transformer);
    }

    std::shared_ptr<Node> getLeft() {
      return _left;
    }
    std::shared_ptr<Node> getRight() {
      return _right;
    }
    void setLeft(std::shared_ptr<Node> left) {
      _left = left;
    }
    void setRight(std::shared_ptr<Node> right) {
      _right = right;
    }

   private:
    T _value;
    std::shared_ptr<Node> _left, _right;
    std::list<std::function<T(T)>> transformers;
  };


 public:
  Tree()
    : _root(nullptr) {}

  explicit Tree(const std::shared_ptr<Node> &root)
    : _root(root) {}

  Tree(const Tree &tree)
    : _root(tree.fold(
        [] (T value, std::shared_ptr<Node> left, std::shared_ptr<Node> right) {
          return createValueNode(value, left, right);
        },
        createEmptyNode()
      )) {}

  template<typename F, typename U>
  U fold(F operation, U init) const {
    return Node::foldNode(_root, operation, init);
  }

  Tree filter(std::function<bool(T)> predicate) const {
    std::shared_ptr<Node> newRoot = fold(
      [predicate] (T value,
                   std::shared_ptr<Node> left,
                   std::shared_ptr<Node> right) {
        if (predicate(value))
          return createValueNode(value, left, right);
        if (!left && !right)
          return std::shared_ptr<Node>();
        if (!left)
          return right;
        if (!right)
          return left;

        std::shared_ptr<Node> tmp = left;
        while (tmp->getRight())
          tmp = tmp->getRight();
        tmp->setRight(right);
        return left;
      },
      createEmptyNode()
    );
    return Tree(newRoot);
  }

  Tree map(std::function<T(T)> transformer) const {
    std::shared_ptr<Node> newRoot = fold(
      [transformer] (T value,
                     std::shared_ptr<Node> left,
                     std::shared_ptr<Node> right) {
        return createValueNode(transformer(value), left, right);
      },
      createEmptyNode()
    );
    return Tree(newRoot);
  }

  Tree lazy_map(std::function<T(T)> transformer) const {
    std::shared_ptr<Node> newRoot = fold(
      [transformer] (T value,
                     std::shared_ptr<Node> left,
                     std::shared_ptr<Node> right) {
        std::shared_ptr<Node> newNode = createValueNode(value, left, right);
        newNode->addTransformer(transformer);
        return newNode;
      },
      createEmptyNode()
    );
    return Tree(newRoot);
  }

  T accumulate(std::function<T(T, T)> operation,
               T init,
               std::function<std::list<T>(const Tree &)> traversal) const {
    std::list<T> values = traversal(*this);
    T currentResult = init;
    for (T value : values)
      currentResult = operation(currentResult, value);
    return currentResult;
  }

  void apply(std::function<void(T)> operation,
             std::function<std::list<T>(const Tree &)> traversal) const {
    std::list<T> values = traversal(*this);
    for (T value : values)
      operation(value);
  }

  int height() const {
    return fold(
      [] (T value, int left, int right) {
        return std::max(left, right) + 1;
      },
      0
    );
  }

  int size() const {
    return fold(
      [] (T value, int left, int right) {
        return left + right + 1;
      },
      0
    );
  }

  bool is_bst() const {
    return std::get<0>(fold(
      [] (T value,
          // is BST, is empty node, min value in subtree, max value in subtree
          std::tuple<bool, bool, int, int> left,
          std::tuple<bool, bool, int, int> right) {
        if (!std::get<0>(left) || !std::get<0>(right) ||
            (!std::get<1>(left) && std::get<3>(left) > value) ||
            (!std::get<1>(right) && std::get<2>(right) < value))
          return std::make_tuple(false, true, 0, 0);
        int newMin = value, newMax = value;
        if (!std::get<1>(left)) {
          newMin = std::min(newMin, std::get<2>(left));
          newMax = std::max(newMax, std::get<3>(left));
        }
        if (!std::get<1>(right)) {
          newMin = std::min(newMin, std::get<2>(right));
          newMax = std::max(newMax, std::get<3>(right));
        }
        return std::make_tuple(true, false, newMin, newMax);
      },
      std::make_tuple(true, true, 0, 0)
    ));
  }

  void print(std::function<std::list<T>(const Tree &)> traversal =
               Tree<T>::inorder) const {
    std::list<T> values = traversal(*this);
    for (T value : values)
      std::cout << value << ' ';
    std::cout << std::endl;
  }

  static std::shared_ptr<Node> createEmptyNode() {
    return std::shared_ptr<Node>(nullptr);
  }

  static std::shared_ptr<Node> createValueNode(T value) {
    return std::make_shared<Node>(Node(value));
  }

  static std::shared_ptr<Node> createValueNode(T value,
                                               std::shared_ptr<Node> left,
                                               std::shared_ptr<Node> right) {
    return std::make_shared<Node>(Node(value, left, right));
  }

  static std::list<T> inorder(const Tree &tree) {
    return tree.fold(
      [] (T value, std::list<T> left, std::list<T> right) {
        left.push_back(value);
        left.splice(left.end(), right);
        return left;
      },
      std::list<T>());
  }

  static std::list<T> postorder(const Tree &tree) {
    return tree.fold(
      [] (T value, std::list<T> left, std::list<T> right) {
        left.splice(left.end(), right);
        left.push_back(value);
        return left;
      },
      std::list<T>());
  }

  static std::list<T> preorder(const Tree &tree) {
    return tree.fold(
      [] (T value, std::list<T> left, std::list<T> right) {
        left.push_front(value);
        left.splice(left.end(), right);
        return left;
      },
      std::list<T>());
  }


 private:
  std::shared_ptr<Node> _root;

};

#endif  // TREE_H_
