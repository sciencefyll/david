#include "david/utils/utils.h"
#include "fann/floatfann.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include "david/types.h"
#include "david/MoveGen.h"
#include <david/ChessEngine.h>
#include <map>

#ifdef __linux__
//linux code goes here
#include <assert.h>
#include <sstream>
#elif _WIN32
// windows code goes here
#endif

namespace utils {

/**
 * Converts a numeric integer string into an int, empty strings included.
 *
 * @param v The std::string value, eg. "123", or "" or " "
 * @return the int value: 123 or 0
 */
inline namespace convert {
int stoi(const std::string v) {
  return v == "" || v == " " ? 0 : ::std::stoi(v);
}
//constexpr int ctoi(const char c) {
//  return c == ' ' ? 0 : c - '0';
//}

}


bool fileExists(const std::string &file) {
  struct stat buffer;
  return (stat(file.c_str(), &buffer) == 0);
}

void yellDeprecated(const std::string info) {
  std::cerr << "@Deprecated function used " << (info.length() == 0 ? "!" : info) << std::endl;
}

/**
 * Prints a bitboard as a chessboard with 0's and 1's
 * @param board - at bitboard_t
 *
63	62	61	60	59	58	57	56
55	54	53	52	51	50	49	48
47	46	45	44	43	42	41	40
39	38	37	36	35	34	33	32
31	30	29	28	27	26	25	24
23	22	21	20	19	18	17	16
15	14	13	12	11	10	09	08
07	06	05	04	03	02	01	00
 *
 *
 */
void printBoard(uint64_t bb, const int index) {
  std::string board(64, '-');

  while (bb != 0ULL) {
    const auto i = LSB(bb);
    flipBitOff(bb, i);
    board.at(63 - i) = 'X';
  }

  // add the index as a O
  if (index > -1) {
    board.at(63 - index) = 'O';
  }

  std::cout << "\n ";
  std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
  for (uint8_t i = 0; i < 8; i++) {
    std::cout << 8 - i << " | ";
    for (uint8_t j = 0; j < 8; j++) {
      std::cout << board.at(63 - ((i * 8) + j)) << " | ";
    }
    std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
  }
  std::cout << "  ";
  for (int i = 'A'; i < 'A' + 8; i++) {
    std::cout << "  " << (char) i << " ";
  }
  std::cout << std::endl;
}


/**
 * Converts indexes such as "E6" into an integer index: H1 == 0.
 *
 * @param chessIndex String such as "E6"
 * @return integer bitboard_t index, 64 on error.
 */
uint8_t chessIndexToArrayIndex(const std::string &chessIndex) {
  uint8_t index = 64;

  // Is always 2 in size "alpha" + "numeric" => "H1"
  if (chessIndex.length() != 2) {
    return index;
  }

  // store first and second char
  const char &a = chessIndex.front();
  const char &b = chessIndex.back();

  char cPos[] = {'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a'}; // matches index value
  char cPos2[] = {'H', 'G', 'F', 'E', 'D', 'C', 'B', 'A'}; // matches index value
  uint8_t first = 0;
  uint8_t second = b - 48 - 1;
  for (int i = 0; i < 8; i++) {
    if (a == cPos[i] || a == cPos2[i]) {
      first = i;
      break;
    }
  }

  index = second * 8 + first;

  if (index > 64) {
    return 64;
  }

  return index;
}


std::string getAbsoluteProjectPath() {
#ifdef __linux__
  //linux code goes here
  //char buffer[PATH_MAX];
  //std::string path = getcwd(buffer, PATH_MAX) ? std::string(buffer) : std::string("");

  if (const char* usr = std::getenv("USER")) {
    std::string username(usr);
    return "/home/" + username + "/.chess_david/"; //folder
  }
  else {
    std::__throw_logic_error("Unable to get username in linux!");
  }

  return "";
#elif _WIN32
  // windows code goes here
  return "";
#elif _WIN64
  // windows code goes here
  return "";
#else
  // idk
  std::cerr << "UNSUPPORTED PLATFORM!!!" << std::endl;
  return "";
#endif
}


void perft() {
  perft(6); // from depth 0 to depth 6, is default.
}

// oh this is bad, how hte first arg is now the max depth
void perft(const uint8_t depth) {
  ::david::type::gameState_t gs;
  ::utils::gameState::setDefaultChessLayout(gs);
  perft(gs, 0, depth);
}

void perft(const uint8_t depth, const std::string FEN, const uint8_t start) {
  ::david::type::gameState_t gs;

  if (FEN.empty()) {
    ::utils::gameState::setDefaultChessLayout(gs);
  }
  else {
    ::utils::gameState::generateFromFEN(gs, FEN);
  }

  ::utils::perft(gs, start < depth ? start : depth, depth);
}

// and here its the start.
// by giving an gs here, we can change the board layouts before perfts.
void perft(const ::david::type::gameState_t& gs, const uint8_t start, const uint8_t end) {
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
  std::printf("| %5s | %30s | %8s |\n",
              "Depth",
              "Nodes",
              "Time");
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
  for (uint8_t depth = start; depth <= end; depth++) {
    // start time
    struct timeval tp, tp2;
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    // get nodes for given depth
    uint64_t nodes = depth == 0 ? 1 : ::utils::perft(depth, gs);
    const auto prettyNodeCount = ::utils::prettyNum(nodes);

    // time finished
    gettimeofday(&tp2, NULL);
    long int ms2 = tp2.tv_sec * 1000 + tp2.tv_usec / 1000;

    // print results
    std::printf("| %5i | %30llu | %8.2f |\n",
                  depth, nodes,  (ms2 - ms1) / 1000.0
    );
  }

  // print ending
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
}

/**
 * Assumes that the depth is never 0!
 * @param depth
 * @param gs
 * @return
 */
uint64_t perft(const uint8_t depth, const ::david::type::gameState_t &gs) {
  // create a holder for possible game outputs
  ::david::MoveGen moveGen{gs};

  std::array<::david::type::gameState_t, ::david::constant::MAXMOVES> states;
  const uint16_t len = moveGen.template generateGameStates<::david::constant::MAXMOVES>(states);

  // if depth is 1 just return the length
  if (depth == 1) {
    return len;
  }

  // otherwise continue down the spiral
  uint64_t nodes = 0;
  const uint8_t nextDepth = depth - 1;
  for (unsigned long i = 0; i < len; i++) {
    nodes += perft(nextDepth, states[i]);
  }

  // return amount
  return nodes;
}


// oh this is bad, how hte first arg is now the max depth
// @deprecated, serious error in reading total nr of nodes.
void perft_threaded(const uint8_t depth, const std::string FEN, const uint8_t start) {
  ::david::type::gameState_t gs;

  if (FEN == "" || FEN == " ") {
    ::utils::gameState::setDefaultChessLayout(gs);
  }
  else {
    ::utils::gameState::generateFromFEN(gs, FEN);
  }

  ::utils::perft_threaded(gs, start < depth ? start : depth, depth);
}

// and here its the start.
// by giving an gs here, we can change the board layouts before perfts.
void perft_threaded(const ::david::type::gameState_t& gs, const uint8_t start, const uint8_t end) {
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
  std::printf("| %5s | %30s | %8s |\n",
              "Depth",
              "Nodes",
              "Time");
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
  for (uint8_t depth = start; depth <= end; depth++) {
    // start time
    struct timeval tp, tp2;
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    // get nodes for given depth
    uint64_t nodes = depth == 0 ? 1 : ::utils::perft_threaded(depth, gs);
    const auto prettyNodeCount = ::utils::prettyNum(nodes);

    // time finished
    gettimeofday(&tp2, NULL);
    long int ms2 = tp2.tv_sec * 1000 + tp2.tv_usec / 1000;

    // print results
    std::printf("| %5i | %30s | %8.2f |\n",
                depth, prettyNodeCount.c_str(),  (ms2 - ms1) / 1000.0
    );
  }

  // print ending
  std::printf("+%7s+%32s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------");
}

uint64_t perft_threaded(const uint8_t depth, const ::david::type::gameState_t &gs) {
  // create a holder for possible game outputs
  ::david::MoveGen moveGen{gs};

  std::array<::david::type::gameState_t, ::david::constant::MAXMOVES> states;
  const uint16_t len = moveGen.template generateGameStates<::david::constant::MAXMOVES>(states);

  // if depth is 1 just return the length
  if (depth == 1) {
    return len;
  }

  // otherwise continue down the spiral
  uint64_t nodes = 0;
  const uint8_t nextDepth = depth - 1;

  // thread every possible branch to gain speed
  if (depth % 5 == 0) {
    std::array<std::thread, ::david::constant::MAXMOVES> threads;
    for (unsigned long i = 0; i < len; i++) {
      threads[i] = std::thread([&]() {
        nodes += perft_threaded(nextDepth, states[i]);
      });
    }

    for (unsigned long i = 0; i < len; i++) {
      threads[i].join();
    }
  }
  else {
    for (unsigned long i = 0; i < len; i++) {
      nodes += perft_threaded(nextDepth, states[i]);
    }
  }

  // return amount
  return nodes;
}



// advanced movegen debugging
void perft_advanced(const uint8_t depth, const std::string FEN, const uint8_t start) {
  ::david::type::gameState_t gs;

  if (FEN == "" || FEN == " ") {
    ::utils::gameState::setDefaultChessLayout(gs);
  }
  else {
    ::utils::gameState::generateFromFEN(gs, FEN);
  }

  ::utils::perft_advanced(gs, start < depth ? start : depth, depth, false);
}

bool perft_advanced(const ::david::type::gameState_t& gs, const uint8_t start, const uint8_t end, const bool showEGN) {

  // Display perft for given FEN
  std::cout << "FEN string: " << ::utils::gameState::generateFen(gs) << '\n';

  std::printf("+%7s+%32s+%10s+%10s+%10s+%10s+%10s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------");
  std::printf("| %5s | %30s | %8s | %8s | %8s | %8s | %8s | %8s |\n",
              "Depth",
              "Nodes",
              "Captures",
              "E.P.",
              "Castles",
              "Promos",
              "Checks",
              "Checkm's");
  std::printf("+%7s+%32s+%10s+%10s+%10s+%10s+%10s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------");
  for (uint8_t depth = start; depth <= end; depth++) {
    // run perft
    std::array<unsigned int, 6> perftResults = {}; // zero initialization
    std::map<std::string, unsigned int> moves{};

    auto moveGenPerft = ::utils::perft_advanced(depth, gs, perftResults, moves);
    const auto prettyNodeCount = ::utils::prettyNum(moveGenPerft);

    std::printf("| %5i | %30llu | %8i | %8i | %8i | %8i | %8i | %8i |\n",
                depth,
                moveGenPerft,
                perftResults[0],
                perftResults[1],
                perftResults[2],
                perftResults[3],
                perftResults[4],
                perftResults[5]);
//                ::utils::prettyNum(moveGenPerft).c_str(),
//                ::utils::prettyNum(perftResults[0]).c_str(),
//                ::utils::prettyNum(perftResults[1]).c_str(),
//                ::utils::prettyNum(perftResults[2]).c_str(),
//                ::utils::prettyNum(perftResults[3]).c_str(),
//                ::utils::prettyNum(perftResults[4]).c_str(),
//                ::utils::prettyNum(perftResults[5]).c_str());

    //for (const auto& [egn, count] : moves) {
    //  std::cout << egn << ": " << count << std::endl;
    //}
  }

  std::printf("+%7s+%32s+%10s+%10s+%10s+%10s+%10s+%10s+\n",
              "-------",
              "--------------------------------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------",
              "----------");
}

uint64_t perft_advanced(const uint8_t depth, const ::david::type::gameState_t &gs, std::array<unsigned int, 6>& results, std::map<std::string, unsigned int>& moves) {
  if (depth == 0) {
    return 1;
  }

  // create a holder for possible game outputs
  ::david::MoveGen moveGen{gs};

  std::array<::david::type::gameState_t, ::david::constant::MAXMOVES> states;
  const uint16_t len = moveGen.template generateGameStates<::david::constant::MAXMOVES>(states);
  uint64_t nodes = 0;

  // calculate move for every move
  const uint8_t nextDepth = depth - 1;
  for (unsigned long i = 0; i < len; i++) {
    auto& state = states[i];

    if (depth == 1) {

      // add move
      const std::string egn = ::utils::gameState::getEGN(gs, state);
      if (moves.count(egn) == 0) {
        moves[egn] = 1;
      }
      else {
        moves[egn] += 1;
      }


      // add any captures
      if (((gs.piecess[1] & state.piecess[1]) > 0 || state.passant)
          && ::utils::nrOfActiveBits(gs.piecess[0] ^ state.piecess[1]) == 2
          && ::utils::nrOfActiveBits(gs.piecess[1] ^ state.piecess[0]) == 1
          ) {
        results[0] += 1;

        // en passant
        if (state.passant) {
          results[1] += 1;
        }
      }

      // castling
#ifdef DAVID_TEST
      if (state.castled) {
        results[2] += 1;
      }
#else
      else if ((gs.kingCastlings[0] != state.kingCastlings[1]) || (gs.queenCastlings[0] != state.queenCastlings[1])) {
        // castling state has changed, but did the king move?
        // check if the king has moved more than one position.
        if ((gs.piecesArr[5][0] & 576460752303423496) > 0 && (state.piecesArr[5][1] & 2449958197289549858) > 0) {
          // check that rook also moved
          if ((gs.piecesArr[::david::constant::index::rook][0] & 9295429630892703873) > 0 && (state.piecesArr[::david::constant::index::rook][1] & 1441151880758558740) > 0) {
            results[2] += 1;
          }
        }
      }
#endif

      // promotion
#ifdef DAVID_TEST
      if (state.promoted) {
        results[3] += 1;
      }
#else
      // check if a pawn has been reduced by one, but that nr of total pieces hasn't been reduced
      if (
          ::utils::nrOfActiveBits(gs.piecesArr[0][0]) - ::utils::nrOfActiveBits(state.piecesArr[0][1]) == 1 &&
              ::utils::nrOfActiveBits(gs.piecess[0]) == ::utils::nrOfActiveBits(state.piecess[1])
          ) {
        results[3] += 1;
      }
#endif
      // check, but also check mate
#ifdef DAVID_TEST
      if (state.isInCheck) {
        results[4] += 1;

        // to see if the state is in check mate, lets just see how many moves it can generate
        ::david::MoveGen mg{state};
        if (mg.nrOfPossibleMoves() == 0) {
          results[5] += 1; // unable to generate any valid moves, aka check mate.
        }
      }
#endif

      nodes += 1;
    }
    else {
      nodes += ::utils::perft_advanced(nextDepth, state, results, moves);
    }
  }

  return nodes;
}


} // End of utils

