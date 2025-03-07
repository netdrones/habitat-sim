// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#ifndef ESP_CORE_MANAGEDFILEBASEDCONTAINER_H_
#define ESP_CORE_MANAGEDFILEBASEDCONTAINER_H_

/** @file
 * @brief Class Template @ref esp::core::ManagedFileBasedContainer : @ref
 * esp::core::ManagedContainer functionality specifically for file-based @ref
 * esp::core::AbstractManagedObject objects
 */

#include "AbstractFileBasedManagedObject.h"
#include "ManagedContainer.h"
#include "esp/io/Json.h"

#include <Corrade/Utility/Directory.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/String.h>
#include <typeinfo>
namespace Cr = Corrade;

namespace esp {
namespace core {

namespace managedContainers {
/**
 * @brief Class template defining file-io-based responsibilities and
 * functionality for managing @ref esp::core::AbstractFileBasedManagedObject
 * constructs.
 * @tparam T the type of managed object a particular specialization of
 * this class works with.  Must inherit from @ref
 * esp::core::AbstractFileBasedManagedObject.
 * @tparam Access Whether the default access (getters) for this
 * container provides copies of the objects held, or the actual objects
 * themselves.
 */
template <class T, ManagedObjectAccess Access>
class ManagedFileBasedContainer : public ManagedContainer<T, Access> {
 public:
  static_assert(
      std::is_base_of<AbstractFileBasedManagedObject, T>::value,
      "ManagedFileBasedContainer :: Managed object type must be derived from "
      "AbstractFileBasedManagedObject");
  typedef std::shared_ptr<T> ManagedFileIOPtr;

  explicit ManagedFileBasedContainer(const std::string& metadataType,
                                     const std::string& JSONTypeExt)
      : ManagedContainer<T, Access>(metadataType), JSONTypeExt_(JSONTypeExt) {}

  /**
   * @brief Creates an instance of a managed object from a JSON file.
   *
   * @param filename the name of the file describing the object managed object.
   * Assumes it exists and fails if it does not.
   * @param registerObject whether to add this managed object to the
   * library. If the user is going to edit this managed object, this should be
   * false - any subsequent editing will require re-registration. Defaults to
   * true.
   * @return a reference to the desired managed object, or nullptr if fails.
   */
  ManagedFileIOPtr createObjectFromJSONFile(const std::string& filename,
                                            bool registerObject = true) {
    std::unique_ptr<io::JsonDocument> docConfig{};
    bool success = this->verifyLoadDocument(filename, docConfig);
    if (!success) {
      ESP_ERROR() << "<" << Magnum::Debug::nospace << this->objectType_
                  << Magnum::Debug::nospace
                  << "> : Failure reading document as JSON :" << filename
                  << ". Aborting.";
      return nullptr;
    }
    // convert doc to const value
    const io::JsonGenericValue config = docConfig->GetObject();
    ManagedFileIOPtr attr = this->buildManagedObjectFromDoc(filename, config);
    return this->postCreateRegister(attr, registerObject);
  }  // ManagedFileBasedContainer::createObjectFromJSONFile

  /**
   * @brief Method to load a Managed Object's data from a file.  If the file
   * type is not supported by specialization of this method, this method
   * executes and an error is thrown.
   * @tparam type of document to load.
   * @param filename name of file document to load from
   * @param config document to read for data
   * @return a shared pointer of the created Managed Object
   */
  template <typename U>
  ManagedFileIOPtr buildManagedObjectFromDoc(const std::string& filename,
                                             CORRADE_UNUSED const U& config) {
    ESP_ERROR()
        << "<" << Magnum::Debug::nospace << this->objectType_
        << Magnum::Debug::nospace
        << "> : Failure loading attributes from document of unknown type :"
        << filename << ". Aborting.";
  }
  /**
   * @brief Method to load a Managed Object's data from a file.  This is the
   * JSON specialization, using type inference.
   * @param filename name of file document to load from
   * @param config JSON document to read for data
   * @return a shared pointer of the created Managed Object
   */
  ManagedFileIOPtr buildManagedObjectFromDoc(
      const std::string& filename,
      const io::JsonGenericValue& jsonConfig) {
    return this->buildObjectFromJSONDoc(filename, jsonConfig);
  }

  /**
   * @brief Parse passed JSON Document specifically for @ref ManagedPtr object.
   * It always returns a @ref ManagedPtr object.
   * @param filename The name of the file describing the @ref ManagedPtr,
   * used as managed object handle/name on create.
   * @param jsonConfig json document to parse - assumed to be legal JSON doc.
   * @return a reference to the desired managed object.
   */
  virtual ManagedFileIOPtr buildObjectFromJSONDoc(
      const std::string& filename,
      const io::JsonGenericValue& jsonConfig) = 0;

  /**
   * @brief Saves the @ref esp::core::AbstractFileBasedManagedObject with handle
   * @p objectHandle to a JSON file using a non-colliding version (if @p
   * overwrite is false) the object's handle, with appropriate extension
   * denoting type of JSON, as file name, to the @ref
   * esp::core::AbstractFileBasedManagedObject's specified file directory.
   * @param objectHandle The name of the object to save. If not found, returns
   * false.
   * @param overwrite Whether or not an existing json file with the same name
   * should be overwritten.
   * @return Whether save was successful
   */

  bool saveManagedObjectToFile(const std::string& objectHandle, bool overwrite);

  /**
   * @brief Saves the @ref esp::core::AbstractFileBasedManagedObject with handle
   * @p objectHandle to a JSON file using the specified, fully-qualified @p
   * fullFilename, with appropriate type extension appended if not present. Will
   * overwrite any file with same name found.
   * @param objectHandle The name of the object to save. If not found, returns
   * false.
   * @param fullFilename The name of the file to save to.  Will overwrite any
   * file that has the same name.
   * @return Whether save was successful
   */
  bool saveManagedObjectToFile(const std::string& objectHandle,
                               const std::string& fullFilename) {
    if (!this->getObjectLibHasHandle(objectHandle)) {
      // Object not found
      ESP_ERROR()
          << "<" << this->objectType_
          << ">::saveManagedObjectToFile : No object exists with handle "
          << objectHandle << " to save as JSON. Aborting.";
      return false;
    }

    // Managed file-based object to save
    ManagedFileIOPtr obj = this->template getObjectInternal<T>(objectHandle);
    namespace FileUtil = Cr::Utility::Directory;

    std::string fileDirectory = FileUtil::path(fullFilename);
    // if no directory given then use object's local directory
    if (fileDirectory.empty()) {
      fileDirectory = obj->getFileDirectory();
    }
    // construct filename candidate from given fully qualified filename
    // This will make sure written file will have appropriate extension
    const std::string fileName =
        FileUtil::splitExtension(
            FileUtil::splitExtension(FileUtil::filename(fullFilename)).first)
            .first +
        "." + this->JSONTypeExt_;

    return this->saveManagedObjectToFileInternal(obj, fileName, fileDirectory);

  }  // ManagedFileBasedContainer::saveManagedObjectToFile

  /**
   * @brief Return a properly formated JSON file name for the @ref
   * esp::core::AbstractFileBasedManagedObject managed by this manager.  This
   * will change the extension to the appropriate json extension.
   * @param filename The original filename
   * @return a candidate JSON file name for the @ref
   * esp::core::AbstractFileBasedManagedObject managed by this manager.
   */
  std::string getFormattedJSONFileName(const std::string& filename) {
    return this->convertFilenameToPassedExt(filename, this->JSONTypeExt_);
  }

  /**
   * @brief Returns the config file type and file extension used for the files
   * that build the @ref esp::core::AbstractFileBasedManagedObject managed by
   * this manager.
   */
  std::string getJSONTypeExt() const { return JSONTypeExt_; }

 protected:
  //======== Common File-based import and utility functions ========

  /**
   * @brief Saves @p managedObject to a JSON file using the given @p
   * fileName in the given @p fileDirectory .
   * @param managedObject The name of the object to save. If not found, returns
   * false.
   * @param filename The filename of the file to save to.
   * @param fileDirectory The directory to save to. If the directory does not
   * exist, will return false.
   * @return Whether save was successful
   */
  virtual bool saveManagedObjectToFileInternal(
      const ManagedFileIOPtr& managedObject,
      const std::string& filename,
      const std::string& fileDirectory) const = 0;

  /**
   * @brief Verify passd @p filename is legal document of type T. Returns loaded
   * document in passed argument if successful. This requires appropriate
   * specialization for each type name, so if this method is executed it means
   * no appropriate specialization exists for passed type of document.
   *
   * @tparam type of document
   * @param filename name of potential document to load
   * @param resDoc a reference to the document to be parsed.
   * @return whether document has been loaded successfully or not
   */
  template <class U>
  bool verifyLoadDocument(const std::string& filename,
                          CORRADE_UNUSED std::unique_ptr<U>& resDoc) {
    // by here always fail - means document type U is unsupported
    ESP_ERROR() << "<" << Magnum::Debug::nospace << this->objectType_
                << Magnum::Debug::nospace << "> : File" << filename
                << "failed due to unsupported file type :" << typeid(U).name();
    return false;
  }  // ManagedContainerBase::verifyLoadDocument
  /**
   * @brief Verify passed @p filename is legal json document, return loaded
   * document or nullptr if fails
   *
   * @param filename name of potential json document to load
   * @param jsonDoc a reference to the json document to be parsed
   * @return whether document has been loaded successfully or not
   */
  bool verifyLoadDocument(const std::string& filename,
                          std::unique_ptr<io::JsonDocument>& jsonDoc);

  /**
   * @brief Will build a new file name for @p filename by replacing the existing
   * extension(s) with the passed @p fileTypeExt, if it is missing.  NOTE : this
   * does not verify that file exists.
   * @param filename The original file name
   * @param fileTypeExt The extension to use for the new filename.
   * @return The file name changed so that it has the correct @p fileTypeExt if
   * it was missing.
   */
  std::string convertFilenameToPassedExt(const std::string& filename,
                                         const std::string& fileTypeExt);

  /**
   * @brief Get directory component of managed object handle and call @ref
   * esp::core::AbstractManagedObject::setFileDirectory if a legitimate
   * directory exists in handle.
   *
   * @param object pointer to managed object to set
   */
  void setFileDirectoryFromHandle(ManagedFileIOPtr object) {
    std::string handleName = object->getHandle();
    auto loc = handleName.find_last_of('/');
    if (loc != std::string::npos) {
      object->setFileDirectory(handleName.substr(0, loc));
    }
  }  // setFileDirectoryFromHandle

  // ======== Instance Variables ========
  /**
   * @brief The string extension for the JSON configuration file backing this
   * Manager's @ref esp::core::managedContainers::AbstractFileBasedManagedObject
   * including the json extension.
   */
  const std::string JSONTypeExt_;

 public:
  ESP_SMART_POINTERS(ManagedFileBasedContainer<T, Access>)

};  // class ManagedFileBasedContainer

/////////////////////////////
// Class Template Method Definitions

template <class T, ManagedObjectAccess Access>
std::string ManagedFileBasedContainer<T, Access>::convertFilenameToPassedExt(
    const std::string& filename,
    const std::string& fileTypeExt) {
  std::string strHandle = Cr::Utility::String::lowercase(filename);
  std::string resHandle(filename);
  // If filename does not already have extension of interest
  if (strHandle.find(Cr::Utility::String::lowercase(fileTypeExt)) ==
      std::string::npos) {
    resHandle = Cr::Utility::Directory::splitExtension(filename).first + "." +
                fileTypeExt;
    ESP_VERY_VERBOSE() << "<" << Magnum::Debug::nospace << this->objectType_
                       << Magnum::Debug::nospace << "> : Filename :" << filename
                       << "changed to proposed" << fileTypeExt
                       << "filename :" << resHandle;
  } else {
    ESP_VERY_VERBOSE() << "<" << Magnum::Debug::nospace << this->objectType_
                       << Magnum::Debug::nospace << "> : Filename :" << filename
                       << "contains requested file extension" << fileTypeExt
                       << "already.";
  }
  return resHandle;
}  // ManagedFileBasedContainer<T, Access>::convertFilenameToPassedExt

template <class T, ManagedObjectAccess Access>
bool ManagedFileBasedContainer<T, Access>::verifyLoadDocument(
    const std::string& filename,
    std::unique_ptr<io::JsonDocument>& jsonDoc) {
  if (Cr::Utility::Directory::exists(filename)) {
    try {
      jsonDoc = std::make_unique<io::JsonDocument>(io::parseJsonFile(filename));
    } catch (...) {
      ESP_ERROR() << "<" << Magnum::Debug::nospace << this->objectType_
                  << Magnum::Debug::nospace << "> : Failed to parse" << filename
                  << "as JSON.";
      return false;
    }
    return true;
  } else {
    // by here always fail
    ESP_ERROR() << "<" << Magnum::Debug::nospace << this->objectType_
                << Magnum::Debug::nospace << "> : File" << filename
                << "does not exist";
    return false;
  }
}  // ManagedFileBasedContainer<T, Access>::verifyLoadDocument

template <class T, ManagedObjectAccess Access>
bool ManagedFileBasedContainer<T, Access>::saveManagedObjectToFile(
    const std::string& objectHandle,
    bool overwrite) {
  if (!this->getObjectLibHasHandle(objectHandle)) {
    // Object not found
    ESP_ERROR() << "<" << this->objectType_
                << ">::saveManagedObjectToFile : No object exists with handle "
                << objectHandle << " to save as JSON. Aborting.";
    return false;
  }
  namespace FileUtil = Cr::Utility::Directory;
  // Managed file-based object to save
  ManagedFileIOPtr obj = this->template getObjectInternal<T>(objectHandle);
  // get file directory
  const std::string fileDirectory = obj->getFileDirectory();
  // get candidate for file name
  // first strip object's file directory from objectHandle
  std::size_t pos = objectHandle.find(fileDirectory);
  std::string fileNameRaw;
  if (pos == std::string::npos) {
    // directory not found, construct filename from simplified object handle
    fileNameRaw = FileUtil::filename(objectHandle);
  } else {
    // directory found, strip it out and leave remainder (including potential
    // subdirs within directory)
    fileNameRaw = objectHandle.substr(pos + fileDirectory.length());
  }
  std::string fileNameBase =
      FileUtil::splitExtension(FileUtil::splitExtension(fileNameRaw).first)
          .first;
  std::string fileName = fileNameBase + "." + this->JSONTypeExt_;
  if (!overwrite) {
    // if not overwrite, then attempt to find a non-conflicting name before
    // attempting to save
    bool nameExists = true;
    int count = 0;
    while (nameExists) {
      nameExists = FileUtil::exists(FileUtil::join(fileDirectory, fileName));
      if (nameExists) {
        // build a new file name candidate by adding copy plus some integer
        // value
        fileName = Cr::Utility::formatString(
            "{} (copy {:04d}).{}", fileNameBase, count, this->JSONTypeExt_);
        ++count;
      }
    }
  }
  return this->saveManagedObjectToFileInternal(obj, fileName, fileDirectory);
}  // ManagedFileBasedContainer<T, Access>::saveManagedObjectToFile
}  // namespace managedContainers
}  // namespace core
}  // namespace esp
#endif  // ESP_CORE_MANAGEDFILEBASEDCONTAINER_H_
