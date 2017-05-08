#include "fann/fann.h"
#include "fann/fann_cpp.h"
#include "chess_ann/utils.h"

#include <ios>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include <chess_ann/environment.h>
#include <chess_ann/GameTree.h>
#include "stockfish/stockfishMock.h"
using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::left;
using std::right;
using std::showpos;
using std::noshowpos;


// Callback function that simply prints the information to cout
int print_callback(FANN::neural_net &net, FANN::training_data &train,
                   unsigned int max_epochs, unsigned int epochs_between_reports,
                   float desired_error, unsigned int epochs, void *user_data)
{
  cout << "Epochs     " << setw(8) << epochs << ". "
       << "Current Error: " << left << net.get_MSE() << right << endl;
  return 0;
}

// Test function that demonstrates usage of the fann C++ wrapper
void xor_test(
    const float learning_rate,
    const unsigned int num_layers,
    const unsigned int num_input,
    const unsigned int num_hidden,
    const unsigned int num_output,
    const float desired_error,
    const unsigned int max_iterations,
    const unsigned int iterations_between_reports
)
{
  cout << endl << "training started." << endl;


  cout << endl << "Creating network." << endl;

  FANN::neural_net net;
  net.create_standard(num_layers, num_input, num_hidden, num_output);

  net.set_learning_rate(learning_rate);

  net.set_activation_steepness_hidden(1.0);
  net.set_activation_steepness_output(1.0);

  net.set_activation_function_hidden(FANN::SIGMOID_SYMMETRIC_STEPWISE);
  net.set_activation_function_output(FANN::SIGMOID_SYMMETRIC_STEPWISE);

  // Set additional properties such as the training algorithm
  //net.set_training_algorithm(FANN::TRAIN_QUICKPROP);

  // Output network type and parameters
  cout << endl << "Network Type                         :  ";
  switch (net.get_network_type())
  {
    case FANN::LAYER:
      cout << "LAYER" << endl;
      break;
    case FANN::SHORTCUT:
      cout << "SHORTCUT" << endl;
      break;
    default:
      cout << "UNKNOWN" << endl;
      break;
  }
  net.print_parameters();

  cout << endl << "Training network." << endl;


  FANN::training_data data;
  if (data.read_train_from_file(::utils::getAbsoluteProjectPath() + "/engine/ANNTraining/trainingdata/fenAndStockfishScore.data")) //hacky fix
  {
    // Initialize and train the network with the data
    net.init_weights(data);

    cout << "Max Epochs " << setw(8) << max_iterations << ". "
         << "Desired Error: " << left << desired_error << right << endl;
    net.set_callback(print_callback, NULL);
    net.train_on_data(data, max_iterations,
                      iterations_between_reports, desired_error);

    cout << endl << "Testing network." << endl;

    for (unsigned int i = 0; i < data.length_train_data(); ++i)
    {
      // Run the network on the test data
      fann_type *calc_out = net.run(data.get_input()[i]);

      cout << "Stockfish test (" << showpos << data.get_input()[i][0] << ", "
           << data.get_input()[i][1] << ") -> " << *calc_out
           << ", should be " << data.get_output()[i][0] << ", "
           << "difference = " << noshowpos
           << fann_abs(*calc_out - data.get_output()[i][0]) << endl;
    }

    cout << endl << "Saving network." << endl;

    // Save the network in floating point and fixed point
    net.save(::utils::getAbsoluteProjectPath() + "/engine/ANNTraining/trainingdata/output/xor_float.net");
    unsigned int decimal_point = net.save_to_fixed(::utils::getAbsoluteProjectPath() + "/engine/ANNTraining/trainingdata/output/xor_fixed.net");
    data.save_train_to_fixed(::utils::getAbsoluteProjectPath() + "/engine/ANNTraining/trainingdata/output/xor_fixed.data", decimal_point);

    cout << endl << "XOR test completed." << endl;
  }
}


std::string getStockfishScore(std::string fen) {
  return std::to_string(::stockfishMock::evaluate(fen));
}

void generateTrainingFile(std::string in, std::string out,

                          const unsigned int max_iterations,
                          const unsigned int num_input,
                          const unsigned int num_output) {
  std::ifstream infile(in);
  std::ofstream output;
  output.open("BUFFER_" + out, std::ios::out | std::ios::trunc);

  int trainingPairs = 0;
  ::environment::Environment env(::bitboard::COLOR::WHITE);

  std::string line;
  while (std::getline(infile, line) && max_iterations > trainingPairs)
  {
    if (line.length() > 5) {

      // write this to a file
      if (output.is_open()) {
        auto score = getStockfishScore(line);
        ::gameTree::nodePtr node = env.generateBoardFromFen(line);

        output << node->BlackBishop << ' ';
        output << node->BlackKing << ' ';
        output << node->BlackKnight << ' ';
        output << node->BlackPawn << ' ';
        output << node->BlackQueen << ' ';
        output << node->BlackRook << ' ';
        output << node->WhiteBishop << ' ';
        output << node->WhiteQueen << ' ';
        output << node->WhiteKnight << ' ';
        output << node->WhitePawn << ' ';
        output << node->WhiteRook << ' ';
        output << node->WhiteKing << std::endl;

        output << score << std::endl;
        trainingPairs += 1;
      }
    }
  }
  infile.close();
  output.close();

  // update file info
  std::ifstream fromBufferFile("BUFFER_" + out);
  std::ofstream outputUpdate(out, std::ios::out | std::ios::trunc);
  if (output.is_open()) {
    outputUpdate << std::to_string(trainingPairs) << " " << std::to_string(num_input) << " " << std::to_string(num_output) << std::endl;
    outputUpdate << fromBufferFile.rdbuf();
    outputUpdate.close();
  }
  fromBufferFile.close();

}



int main (int argc, char * argv[])
{

  const float learning_rate = 0.7f;
  const unsigned int num_layers = 3;
  const unsigned int num_input = 2;
  const unsigned int num_hidden = 3;
  const unsigned int num_output = 1;
  const float desired_error = 0.001f;
  const unsigned int max_iterations = 300000;
  const unsigned int iterations_between_reports = 1000;

  generateTrainingFile("trainingdata/fenstring.txt", "trainingdata/fenAndStockfishScore.data", max_iterations, num_input, num_output);

  try
  {
    std::ios::sync_with_stdio(); // Syncronize cout and printf output
    //xor_test();
  }
  catch (...)
  {
    cerr << endl << "Abnormal exception." << endl;
  }

  // Close program with exit code 0.
  return 0;
}