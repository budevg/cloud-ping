#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>

#include "stat_gen.h"
#include "logging.h"
#include "errors.h"

namespace po = boost::program_options;
using std::vector;
using std::string;
using std::cout;

static void Help(const po::options_description &opts)
{
  cout << "Usage:\n";
  cout << "cloud-ping [options] URL\n";
  cout << opts;
  exit(1);
}

static void ParseProgramOptions(int argc, char **argv, po::variables_map *vm)
{
  po::options_description opts("Options");
  opts.add_options()
    ("repeat,t", "Ping the specified URL until stopped.\n"
                 "To see statistics and continue - type Control-Break;\n"
                 "To stop - type Control-C.")
    ("count,n", po::value<int>(), "Send 'count' requests. Supercedes -t.")
    ("range,r", po::value<string>()->default_value(":"), "Specify range [start, end) of the request data 0:1024, 100:, :1024")
    ("length,l", po::value<size_t>()->default_value(0), "Limit received data to 'length' bytes'")
    ("interval,i", po::value<int>(), "Wait 'interval' seconds between each request. "
                                     "There is a 1-second wait if this option is not specified.")
    ("verbose,v", "Verbose. Print detailed output. Supercedes -s.")
    ("auth,a", po::value<string>()->default_value(""),
     "Authentication string.\n"
     "For S3 '<access-key>:<secret-key>'\n"
     "For Cloud Front '<key_pair_id>:<priv_key_path>'")
    ("url", po::value<vector<string>>(),
     "url to access\n"
     "For http: 'http://some-server.com/file1'\n"
     "For S3: 's3://test-bucket/file1'\n"
     "For Cloud Front 'cf://dgdfdf3b.cloudfront.net/1.bin'")
    ("help,h", "Display help")
    ;

  po::positional_options_description positional_opts;
  positional_opts.add("url", -1);

  try {
    po::store(po::command_line_parser(argc, argv).
              options(opts).positional(positional_opts).run(), *vm);
  }
  catch(std::exception& e) {
    cout << "error: " << e.what() << "\n";
    Help(opts);
  }
  catch(...) {
    cout << "Exception of unknown type!\n";
    Help(opts);
  }

  if (vm->count("help") != 0) {
    Help(opts);
  }

  if (vm->count("url") == 0) {
    cout << "Missing url parameter\n";
    Help(opts);
  }
}

static void ParseRange(const string& range,
                       uint64_t *start,
                       uint64_t *end)
{
  *start = std::numeric_limits<uint64_t>::max();
  *end = std::numeric_limits<uint64_t>::max();
  if (!range.find(":")) {
    return;
  }
  string range_start = range.substr(0, range.find(":"));
  if (range_start != "") {
    *start = stoll(range_start);
  }
  string range_end = range.substr(range.find(":")+1);
  if (range_end != "") {
    *end = stoll(range_end);
  }
}

int main(int argc, char *argv[])
{
  po::variables_map vm;
  ParseProgramOptions(argc, argv, &vm);

  LogLevel log_level = LOG_ERROR;
  if (vm.count("verbose") != 0) {
    log_level = LOG_INFO;
  }
  log_set_level(log_level);


  int interval = 1;
  if (vm.count("interval") != 0) {
    interval = vm["interval"].as<int>();
  }

  int count = 1;
  if (vm.count("count") != 0) {
    count = vm["count"].as<int>();
  }

  bool repeat = false;
  if (vm.count("repeat") != 0) {
    repeat = true;
  }

  uint64_t range_start;
  uint64_t range_end;
  ParseRange(vm["range"].as<string>(),
             &range_start,
             &range_end);

  StatGenerator gen;
  string auth = vm["auth"].as<string>();
  for (auto url: vm["url"].as<vector<string>>()) {
    gen.AddConnection(url, auth, range_start, range_end,
                      vm["length"].as<size_t>());
  }

  if (HttpReq::Init() != RET_OK) {
    log_error("Failed to initialize http request infrastructure");
    return 0;
  }

  gen.Run(count, interval, repeat);
  HttpReq::Fini();
  return 0;
}
