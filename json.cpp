#include <rapidjson/document.h>

using namespace rapidjson;

static double get_time() {
   #if _WIN32 // Fuck I hate windows jesus chrsit.
    LARGE_INTEGER LoggedTime, Frequency;
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&LoggedTime);
    return LoggedTime.QuadPart / (double)Frequency.QuadPart;
  #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
  #endif
}

int main(int argc, char* argv[]) {
  FILE* file = fopen("/home/adam/Work/search-core/t/products250.json", "rb");
  fseek(file, 0, SEEK_END);
  long long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* buffer = new char[length+1];
  fread(buffer, sizeof(char), length, file);
  fclose(file);
  buffer[length] = 0;
  Document document;
  fprintf(stderr, "TIME: %f\n", get_time());
  document.Parse(buffer);
  fprintf(stderr, "TIME: %f\n", get_time());
  return 0;
}
