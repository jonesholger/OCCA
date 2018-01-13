/* The MIT License (MIT)
 *
 * Copyright (c) 2014-2018 David Medina and Tim Warburton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#ifndef OCCA_TOOLS_IO_HEADER
#define OCCA_TOOLS_IO_HEADER

#include <iostream>

#include "occa/defines.hpp"
#include "occa/types.hpp"
#include "occa/tools/hash.hpp"
#include "occa/tools/properties.hpp"
#include "occa/parser/types.hpp"

namespace occa {
  // Kernel Caching
  namespace kc {
    extern const std::string parsedSourceFile;
    extern const std::string launchSourceFile;
    extern const std::string launchBinaryFile;
    extern const std::string sourceFile;
    extern const std::string binaryFile;
    extern const std::string infoFile;
  }

  namespace env {
    class envInitializer_t;
  }

  namespace io {
    class hashAndTag {
    public:
      hash_t hash;
      std::string tag;
      inline hashAndTag() {}
      inline hashAndTag(const hash_t &hash_,
                        const std::string &tag_) :
        hash(hash_),
        tag(tag_) {}
    };

    typedef std::map<std::string, hashAndTag> hashMap;
    hashMap& fileLocks();

    //---[ File Openers ]---------------
    class fileOpener {
      friend class occa::env::envInitializer_t;

    private:
      static std::vector<fileOpener*>& getOpeners();
      static fileOpener& defaultOpener();

    public:
      static fileOpener& get(const std::string &filename);
      static void add(fileOpener* opener);

      virtual ~fileOpener();

      virtual bool handles(const std::string &filename) = 0;
      virtual std::string expand(const std::string &filename) = 0;
    };

    //  ---[ Default File Opener ]------
    class defaultFileOpener : public fileOpener {
    public:
      defaultFileOpener();
      virtual ~defaultFileOpener();

      bool handles(const std::string &filename);
      std::string expand(const std::string &filename);
    };
    //  ================================

    //  ---[ OCCA File Opener ]---------
    class occaFileOpener : public fileOpener {
    public:
      occaFileOpener();
      virtual ~occaFileOpener();

      bool handles(const std::string &filename);
      std::string expand(const std::string &filename);
    };
    //  ================================

    //  ---[ Header File Opener ]-------
    class headerFileOpener : public fileOpener {
    public:
      headerFileOpener();
      virtual ~headerFileOpener();

      bool handles(const std::string &filename);
      std::string expand(const std::string &filename);
    };
    //  ================================

    //  ---[ System Header File Opener ]---
    class systemHeaderFileOpener : public fileOpener {
    public:
      systemHeaderFileOpener();
      virtual ~systemHeaderFileOpener();

      bool handles(const std::string &filename);
      std::string expand(const std::string &filename);
    };
    //  ================================
    //==================================

    const std::string& cachePath();
    const std::string& libraryPath();

    void endWithSlash(std::string &dir);
    std::string endWithSlash(const std::string &dir);

    void removeEndSlash(std::string &dir);
    std::string removeEndSlash(const std::string &dir);

    bool isAbsolutePath(const std::string &filename);

    std::string convertSlashes(const std::string &filename);

    std::string filename(const std::string &filename,
                         bool makeAbsolute = true);

    std::string binaryName(const std::string &filename);

    std::string basename(const std::string &filename,
                         const bool keepExtension = true);

    std::string dirname(const std::string &filename);

    std::string extension(const std::string &filename);

    std::string shortname(const std::string &filename);

    strVector filesInDir(const std::string &dir,
                         const unsigned char fileType);

    strVector directories(const std::string &dir);

    strVector files(const std::string &dir);

    char* c_read(const std::string &filename,
                 size_t *chars = NULL,
                 const bool readingBinary = false);

    std::string read(const std::string &filename,
                     const bool readingBinary = false);

    void write(const std::string &filename,
               const std::string &content);

    std::string getFileLock(const std::string &filename,
                            const std::string &tag);
    void clearLocks();

    bool haveHash(const hash_t &hash,
                  const std::string &tag);
    void waitForHash(const hash_t &hash,
                     const std::string &tag);
    void releaseHash(const hash_t &hash,
                     const std::string &tag);
    void releaseHashLock(const std::string &lockDir);

    kernelMetadataMap parseFile(const std::string &filename,
                                const std::string &outputFile,
                                const occa::properties &props);

    std::string removeSlashes(const std::string &str);

    void cache(const std::string &filename,
               std::string source,
               const hash_t &hash);

    void cache(const std::string &filename,
               const char *source,
               const hash_t &hash,
               const bool deleteSource = true);

    std::string cacheFile(const std::string &filename,
                          const std::string &cachedName,
                          const std::string &header = "",
                          const std::string &footer = "");

    std::string cacheFile(const std::string &filename,
                          const std::string &cachedName,
                          const hash_t &hash,
                          const std::string &header = "",
                          const std::string &footer = "");

    void storeCacheInfo(const std::string &filename,
                        const hash_t &hash,
                        const occa::properties &props);

    std::string getLibraryName(const std::string &filename);

    std::string hashFrom(const std::string &filename);

    std::string hashDir(const hash_t &hash);
    std::string hashDir(const std::string &filename,
                        const hash_t &hash = hash_t());
  }
}

#endif
