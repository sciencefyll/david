#include "david/bitboard.h"
#include "david/GameTree.h"
#include <memory>
#include <iostream>
#include <algorithm>
#include <david/environment.h>
#include <david/utils.h>
#include "david/ANN/ANN.h"

namespace david {
/**
 * Constructor
 */
gameTree::GameTree::GameTree(type::engineContext_ptr ctx)
    : engineContextPtr(ctx),
      maxNumberOfNodes(100) {}

/**
 * Constructor
 * @param node std::share_ptr<::bitboard::gameState>, pass by copy(!!)
 */
gameTree::GameTree::GameTree(type::engineContext_ptr ctx, type::gameState_ptr node)
    : engineContextPtr(ctx),
      current(node),
      maxNumberOfNodes(100) {
}

/**
 * Destructor
 */
gameTree::GameTree::~GameTree() {}

/**
 * Remove every node in this class instance memory.
 */
void gameTree::GameTree::reset() {
  this->current.reset();
  this->previous.reset();
  this->maxNumberOfNodes = 100;
}

/**
 * Resets a single node. This also removes it's children if no other pointer
 * is storing their address.
 *
 * @param node std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::reset(type::gameState_ptr node) {
  if (node == nullptr) {
    return;
  }

  node.reset();
}

/**
 * This deletes a nodes children, and potentially the childrens children.
 * @param node std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::resetChildren(type::gameState_ptr node) {
  if (node == nullptr) {
    return;
  }

  for (auto &child : node->children) {
    this->reset(child);
  }
}

/**
 * Swap out the current node with a new one. The old node is stored in case
 * this change is regretted.
 * This does not reset the max allowed nodes.
 *
 * @see ::gameTree::GameTree::regretNewRootNode()
 * @param node std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::newRootNode(type::gameState_ptr node) {
  if (node == nullptr) {
    return;
  }

  //this->previous.reset();
  //this->previous = this->current;
  //this->previous = nullptr; // make it dissapear, is this safe? TODO: validate.
  this->current = node;
}

/**
 * Generates children for a given node, and sorts the children.
 *
 * @param node std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::generateChildren(type::gameState_ptr node) {
  using bitboard::gameState;
  if (node == nullptr) {
    return;
  }

  // find some way to get all the different boards
  // then call generate node
  // you prolly need to change it's parameters
  int livingNodes = this->getNumberOfNodes();

  // mock test
  node->children.resize(0);

  // created a environ instance based on parent node
  environment::Environment env(node->playerColor);
  env.setGameState(node);

  // create a holdere for possible game outputs
  std::vector<gameState> states;

  // generate possible game oputputs
  env.computeGameStates(states);

  // create node pointers, and set some internal data
  for (int i = 0; i < states.size() && i < livingNodes; livingNodes++, i++) {
    this->generateNode(node, states.at(i));
  }

  // once all the nodes are set, we need to sort the children.
  this->sortChildren(node);
}

/**
 * Sorts children of a given node based on score.
 *
 * @param node std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::sortChildren(type::gameState_ptr node) {
  if (node == nullptr) {
    return;
  }

  std::sort(node->children.begin(), node->children.end(),
            [](const type::gameState_ptr a, const type::gameState_ptr b) -> bool {
              return a->score > b->score;
            });
}

/**
 * This is a restriction to keep the generator from generating infinite nodes.
 * The number given here will equal the max allowed number of nodes in memory.
 *
 * @param n std::share_ptr<::bitboard::gameState>
 */
void gameTree::GameTree::setMaxNumberOfNodes(int n) {
  if (n >= 0) {
    this->maxNumberOfNodes = n;
  } else {
    std::cerr << "::gameTree::GameTree::setMaxNumberOfNodes(int n): Integer 'n' can not be smaller than 0."
              << std::endl;
  }
}

/**
 * Retrieve the max allowed number of nodes in memory.
 * This amount is only limited to this instance.
 *
 * @return int from 0 to N, N <= maxNumberOfNodes
 */
int gameTree::GameTree::getMaxNumberOfNodes() {
  return this->maxNumberOfNodes;
}

/**
 * Just starts generating nodes until the max allowed number of nodes has been reached.
 * Also sorts children.
 *
 * I hate this function @deprecated, see generateChildren in stead.
 */
void gameTree::GameTree::generateNodes() {
  using bitboard::gameState;

  if (this->current == nullptr) {
    std::cerr << "Need a node pointer first" << std::endl;
    return;
  }

  type::gameState_ptr node = this->current;
  int nrOfNodes = this->getNumberOfNodes();

  while (nrOfNodes < this->maxNumberOfNodes) {

    environment::Environment env(node->playerColor);
    env.setGameState(node);
    std::vector<gameState> states;
    env.computeGameStates(states);

    int children = 0;
    for (int i = 0; i < states.size() && nrOfNodes < this->maxNumberOfNodes; nrOfNodes++, i++) {
      this->generateNode(node, states.at(i));
      children += 1;
    }
    this->sortChildren(node);

    if (children > 0) {
      //node = node->children.at(0);
    }
  }

}

/**
 * Gives the number of nodes in memory, within this class instance.
 * This is done by looping through every node, except nullptr.
 *
 * @return int from 0 to N, N <= maxNumberOfNodes
 */
int gameTree::GameTree::getNumberOfNodes() {
  using bitboard::gameState;

  if (this->current == nullptr) {
    return 0;
  }

  int count = 1;

  for (std::shared_ptr<gameState> node : this->current->children) {
    this->getNumberOfNodes(node, count);
  }

  return count;
}

/**
 * Private method used for recursive action to count nodes.
 *
 * @param node
 * @return int from 0 to N, N <= maxNumberOfNodes
 */
void gameTree::GameTree::getNumberOfNodes(type::gameState_ptr node, int &count) {
  using bitboard::gameState;

  if (node->children.size() == 0) {
    count += 1;
  }
  for (std::shared_ptr<gameState> child : node->children) {
    this->getNumberOfNodes(child, count);
  }
}

int gameTree::GameTree::getDepth() {
  using bitboard::gameState;

  if (this->current == nullptr) {
    return 0;
  }

  int depth = this->current->gameTreeLevel;

  for (std::shared_ptr<gameState> node : this->current->children) {
    this->getDepth(node, depth);
  }

  return depth;
}
void gameTree::GameTree::getDepth(type::gameState_ptr node, int &depth) {
  using bitboard::gameState;

  if (node == nullptr) {
    return; // this should never fire.
  }

  if (depth < node->gameTreeLevel) {
    depth += 1;
  }

  for (std::shared_ptr<gameState> child : node->children) {
    this->getDepth(child, depth);
  }
}

/**
 * Print score and depth of all nodes
 * @param root
 */
void gameTree::GameTree::printAllScores(type::gameState_ptr root) {
  std::cout << "Node scores: (Game tree level)";
  for (auto child : root->children) {
    if (child->children.empty()) {
      std::cout << ' ' << child->score << '(' << child->gameTreeLevel << ") ";
    } else
      printAllScores(child);
  }
  std::cout << std::endl;
}

namespace gameTree { // why..

/**
 * Generates a child node for a parent and correctly link them, and insert info.
 *
 * @param parent nodePtr
 * @return nodePtr of the new child, however parent will link to this anyways.
 */
type::gameState_ptr GameTree::generateNode(type::gameState_ptr parent, bitboard::gameState child) {
  using bitboard::gameState;
  using bitboard::COLOR::WHITE;
  using bitboard::COLOR::BLACK;
  using bitboard::bitboard_t;
  auto node = std::make_shared<gameState>();

  node->playerColor = child.playerColor;

  node->BlackRook = child.BlackRook;
  node->BlackQueen = child.BlackQueen;
  node->BlackPawn = child.BlackPawn;
  node->BlackKnight = child.BlackKnight;
  node->BlackKing = child.BlackKing;
  node->BlackBishop = child.BlackBishop;
  node->WhiteRook = child.WhiteRook;
  node->WhiteQueen = child.WhiteQueen;
  node->WhitePawn = child.WhitePawn;
  node->WhiteKnight = child.WhiteKnight;
  node->WhiteKing = child.WhiteKing;
  node->WhiteBishop = child.WhiteBishop;
  node->playerColor = child.playerColor;
  node->blackKingCastling = child.blackKingCastling;
  node->blackQueenCastling = child.blackQueenCastling;
  node->whiteKingCastling = child.whiteKingCastling;
  node->whiteQueenCastling = child.whiteQueenCastling;

  //::environment::Environment env(node->playerColor);
  //env.setGameState(node);


  node->gameTreeLevel = parent->gameTreeLevel + 1;
  node->score = 0;
  node->fullMoves = (node->gameTreeLevel + 1) / 2;
  node->halfMoves = parent->halfMoves + 1;
  node->weakParent = parent;

  // Validate half moves
  if (!utils::isHalfMove(parent, node)) {
    node->halfMoves = 0;
  }
  // check if there are any possible moves after this state

  // use ann to get score
  if (this->engineContextPtr->neuralNetworkPtr != nullptr) {
    int score = this->engineContextPtr->neuralNetworkPtr->ANNEvaluate(node, this->engineContextPtr->playerColor);
    node->score = score;
  }

  // set sub possibilities
  environment::Environment env(node->playerColor);
  env.setGameState(node);
  std::vector<gameState> states;
  env.computeGameStates(states);
  node->possibleSubMoves = states.size(); //won't go above 35 or something.


  // add child to parent
  parent->children.push_back(node);

  return node;
}

/**
 * Get the active node pointer in this class instance.
 *
 * @return nodePtr
 */
type::gameState_ptr GameTree::getCurrentNode() {
  return this->current;
}

/**
 * Converts the previous node to the current node.
 * Does not reset maxAllowedNumberOfNodes.
 *
 * @return The new node, which was the old one.
 */
type::gameState_ptr GameTree::regretNewRootNode() {
  if (this->previous == nullptr) {
    return nullptr;
  }

  this->current = this->previous;
  this->previous = nullptr;

  return this->current;
}

}
}