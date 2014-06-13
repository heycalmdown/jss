{
  "targets": [
      {
          "target_name": "jss",
		  "cflags_cc": [ '-fexceptions' ],
          "sources": [
              "./src/hash.cc",
              "./src/json.cc",
              "./src/crc32.cc",
              "./src/mempool.cc",
              "./src/shm.cc",
			  "./src/semaphore.cc",
			  "./src/bitmap.cc",
			  "./src/error.cc",
              "./src/jss.cc"
          ]
      }
  ]
}
