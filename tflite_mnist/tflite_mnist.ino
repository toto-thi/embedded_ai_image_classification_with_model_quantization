#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
//#include "tensorflow/lite/version.h"
#include "model_.h"
#include "mnist.h"

// Globals
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
tflite::ErrorReporter* reporter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
constexpr int kTensorArenaSize = 5000; // Just pick a big enough number
uint8_t tensor_arena[ kTensorArenaSize ] = { 0 };
float* input_buffer = nullptr;

void setup() {
  // Load Model
  static tflite::MicroErrorReporter error_reporter;
  reporter = &error_reporter;
  reporter->Report( "Let's use AI to recognize some numbers!" );

  model = tflite::GetModel( tf_model );
  if( model->version() != TFLITE_SCHEMA_VERSION ) {
    reporter->Report( "Model is schema version: %d\nSupported schema version is: %d", model->version(), TFLITE_SCHEMA_VERSION );
    return;
  }
  
  // Setup our TF runner
  static tflite::AllOpsResolver resolver;
  //static tflite::MicroInterpreter static_interpreter(
  //    model, resolver, tensor_arena, kTensorArenaSize, reporter );
  interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, kTensorArenaSize, nullptr, nullptr);
  
  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if( allocate_status != kTfLiteOk ) {
    reporter->Report( "AllocateTensors() failed" );
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);

  // Save the input buffer to put our MNIST images into
  input_buffer = input->data.f;
}

void bitmap_to_float_array(float* dest, const unsigned char* bitmap) {
  // Populate input_vec with the monochrome 1bpp bitmap
  int pixel = 0;
  for (int y = 0; y < 28; y++) {
    for (int x = 0; x < 28; x++) {
      int B = x / 8; // the Byte # of the row
      int b = x % 8; // the Bit # of the Byte
      dest[pixel] = (bitmap[y * 4 + B] >> (7 - b)) & 0x1 ? 1.0f : 0.0f;
      pixel++;
    }
  }
}

void print_input_buffer(const float* buffer, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (buffer[y * width + x] > 0.5f) {
        Serial.print('#');
      } else {
        Serial.print(' ');
      }
    }
    Serial.println();
  }
}


void loop() {
  // Pick a random test image for input
  const int num_test_images = (sizeof(test_images) / sizeof(*test_images));
  const unsigned char* image = test_images[rand() % num_test_images];
  
  const int image_width = 28; // MNIST images are 28x28 pixels
  const int image_height = 28;

  bitmap_to_float_array(input_buffer, image);
  print_input_buffer(input_buffer, image_width, image_height);

  // Run our model
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    reporter->Report("Invoke failed");
    return;
  }

  float* result = output->data.f;
  reporter->Report("It looks like the number: %d", std::distance(result, std::max_element(result, result + 10)));

  // Wait 1 second before running again
  delay(5000);
}


