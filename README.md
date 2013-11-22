# cloud-ping
Provides statistics about latency and throughput of requests to
object based storage provided by different cloud providers.

# Supported targets:
  - S3
  - Cloud Front
  - Simple HTTP

# Usage
    cloud-ping [options] URL
    Options:
      -t [ --repeat ]          Ping the specified URL until stopped.
                               To see statistics and continue - type Control-Break;
                               To stop - type Control-C.
      -n [ --count ] arg       Send 'count' requests. Supercedes -t.
      -r [ --range ] arg (=:)  Specify range [start, end) of the request data
                               0:1024, 100:, :1024
      -l [ --length ] arg (=0) Limit received data to 'length' bytes'
      -i [ --interval ] arg    Wait 'interval' seconds between each request. There
                               is a 1-second wait if this option is not specified.
      -v [ --verbose ]         Verbose. Print detailed output. Supercedes -s.
      -a [ --auth ] arg        Authentication string.
                               For S3 '<access-key>:<secret-key>'
                               For Cloud Front '<key_pair_id>:<priv_key_path>'
      --url arg                url to access
                               For http: 'http://some-server.com/file1'
                               For S3: 's3://test-bucket/file1'
                               For Cloud Front 'cf://dgdfdf3b.cloudfront.net/1.bin'
      -h [ --help ]            Display help

# Example

    >> cloud-ping -t -a DASDSAFFDGDFG:fsdf4FSDf33fsff33 s3://test-bucket/dir1/file1

    STAT http://s3.amazonaws.com/test-bucket/dir1/file1 (81.218.79.154)
    [1385147818.040111] 1 bytes from s3.amazonaws.com (81.218.79.154): req=1 time=16.6 ms
    [1385147819.220658] 1 bytes from s3.amazonaws.com (81.218.79.154): req=2 time=105 ms
    [1385147820.248047] 1 bytes from s3.amazonaws.com (81.218.79.154): req=3 time=155 ms
    [1385147821.377807] 1 bytes from s3.amazonaws.com (81.218.79.154): req=4 time=171 ms
    ^C


# Dependency:
  sudo apt-get install libboost-program-options-dev
