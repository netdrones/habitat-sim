// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#ifndef ESP_METADATA_MANAGERS_ATTRIBUTESMANAGERBASE_H_
#define ESP_METADATA_MANAGERS_ATTRIBUTESMANAGERBASE_H_

/** @file
 * @brief Class Template @ref esp::metadata::managers::AttributesManager
 */

#include "esp/metadata/attributes/AttributesBase.h"

#include "esp/core/managedContainers/ManagedFileBasedContainer.h"
#include "esp/io/Io.h"

namespace Cr = Corrade;

namespace esp {
namespace core {
namespace managedContainers {
enum class ManagedObjectAccess;
class ManagedFileBasedContainerBase;
}  // namespace managedContainers
}  // namespace core
namespace metadata {
namespace managers {

using core::config::Configuration;
using core::managedContainers::ManagedFileBasedContainer;
using core::managedContainers::ManagedObjectAccess;

/**
 * @brief Class template defining responsibilities and functionality for
 * managing @ref esp::metadata::attributes::AbstractAttributes constructs.
 * @tparam T the type of managed attributes a particular specialization
 * of this class works with.  Must inherit from @ref
 * esp::metadata::attributes::AbstractAttributes.
 * @tparam Access Whether the default access (getters) for this
 * container provides copies of the objects held, or the actual objects
 * themselves.
 */
template <class T, ManagedObjectAccess Access>
class AttributesManager : public ManagedFileBasedContainer<T, Access> {
 public:
  static_assert(std::is_base_of<attributes::AbstractAttributes, T>::value,
                "AttributesManager :: Managed object type must be derived from "
                "AbstractAttributes");

  typedef std::shared_ptr<T> AttribsPtr;

  /**
   * @brief Construct an attributes manager to manage shared pointers of
   * attributes of type T.
   * @param attrType A string describing the type of attributes, for
   * @param JSONTypeExt The attributes JSON file extension, which must be of the
   * form 'XXXXXX.json', where XXXXXX represents the sub extension specific to
   * the managed type of attributes (i.e. "stage_config.json" for configurations
   * describing stages).
   */
  AttributesManager(const std::string& attrType, const std::string& JSONTypeExt)
      : ManagedFileBasedContainer<T, Access>::ManagedFileBasedContainer(
            attrType,
            JSONTypeExt) {}
  ~AttributesManager() override = default;

  /**
   * @brief Load all file-based templates given string list of template file
   * locations.
   *
   * This will take the list of file names specified and load the referenced
   * templates.  It is assumed these files are JSON files currently.
   * @param tmpltFilenames list of file names of templates
   * @param saveAsDefaults Set these templates as un-deletable from library.
   * @return vector holding IDs of templates that have been added
   */
  std::vector<int> loadAllFileBasedTemplates(
      const std::vector<std::string>& tmpltFilenames,
      bool saveAsDefaults);

  /**
   * @brief Load file-based templates for all @ref JSONTypeExt_
   * files from the provided file or directory path.
   *
   * This will take the passed @p path string and either treat it as a file
   * name or a directory, depending on what is found in the filesystem. If @p
   * path does not end with @ref JSONTypeExt_, it will append this and check to
   * see if such a file exists, and load it. It will also check if @p path
   * exists as a directory, and if so will perform a shallow search to find any
   * files ending in @ref JSONTypeExt_ and load those that are found.
   *
   * @param path A global path to configuration files or a directory containing
   * such files.
   * @param saveAsDefaults Set the templates loaded as undeleteable default
   * templates.
   * @return A list of template indices for loaded valid configs
   */
  std::vector<int> loadAllJSONConfigsFromPath(const std::string& path,
                                              bool saveAsDefaults = false) {
    return this->loadAllTemplatesFromPathAndExt(path, this->JSONTypeExt_,
                                                saveAsDefaults);
  }

  /**
   * @brief Load file-based templates for all @p extType files from the provided
   * file or directory path.
   *
   * This will take the passed @p path string and either treat it as a file
   * name or a directory, depending on what is found in the filesystem. If @p
   * path does not end with @p extType, it will append this and check to
   * see if such a file exists, and load it. It will also check if @p path
   * exists as a directory, and if so will perform a shallow search to find any
   * files ending in @p extType and load those that are found.
   *
   * @param path A global path to configuration files or a directory containing
   * such files.
   * @param extType The extension of files to be attempted to be loaded as
   * templates.
   * @param saveAsDefaults Set the templates loaded as undeleteable default
   * templates.
   * @return A list of template indices for loaded valid configs
   */
  std::vector<int> loadAllTemplatesFromPathAndExt(const std::string& path,
                                                  const std::string& extType,
                                                  bool saveAsDefaults = false);

  /**
   * @brief This builds a list of paths to this type of attributes's JSON Config
   * files from the passed @p jsonPaths array element.  It then will load all
   * the configs it finds at each path.
   * @param configDir The directory to use as a root to search in - may be
   * different than the config dir already listed in this manager.
   * @param jsonPaths The json array element
   */

  void buildJSONCfgPathsFromJSONAndLoad(const std::string& configDir,
                                        const io::JsonGenericValue& jsonPaths) {
    this->buildAttrSrcPathsFromJSONAndLoad(configDir, this->JSONTypeExt_,
                                           jsonPaths);
  }

  /**
   * @brief This builds a list of paths to the @p extType files to use to
   * construct templates derived from the passed @p jsonPaths array element.  It
   * then will load all the configs it finds at each path.
   * @param configDir The directory to use as a root to search in - may be
   * different than the config dir already listed in this manager.
   * @param extType The extension of files to be attempted to be loaded as
   * templates.
   * @param jsonPaths The json array element
   */
  void buildAttrSrcPathsFromJSONAndLoad(const std::string& configDir,
                                        const std::string& extType,
                                        const io::JsonGenericValue& jsonPaths);

  /**
   * @brief Check if currently configured primitive asset template library has
   * passed handle.
   * @param handle String name of primitive asset attributes desired
   * @return whether handle exists or not in asset attributes library
   */
  virtual bool isValidPrimitiveAttributes(const std::string& handle) = 0;

  /**
   * @brief Parse passed JSON Document for @ref
   * esp::metadata::attributes::AbstractAttributes. It always returns a
   * valid @ref esp::metadata::attributes::AbstractAttributes shared
   * pointer.
   *
   * @param templateName The desired name for this @ref
   * esp::metadata::attributes::AbstractAttributes template.
   * @param jsonConfig json document to parse
   * @return a reference to the desired template.
   */
  AttribsPtr buildObjectFromJSONDoc(
      const std::string& templateName,
      const io::JsonGenericValue& jsonConfig) override {
    // Construct a ObjectAttributes and populate with any
    // AbstractObjectAttributes fields found in json.
    auto attributes = this->initNewObjectInternal(templateName, true);
    // set the values for this attributes from the json config.
    this->setValsFromJSONDoc(attributes, jsonConfig);
    return attributes;
  }  // AttributesManager<T, Access>::buildObjectFromJSONDoc

  /**
   * @brief Method to take an existing attributes and set its values from passed
   * json config file.
   * @param attribs (out) an existing attributes to be modified.
   * @param jsonConfig json document to parse
   */
  virtual void setValsFromJSONDoc(AttribsPtr attribs,
                                  const io::JsonGenericValue& jsonConfig) = 0;

  /**
   * @brief This function takes the json block specifying user-defined values
   * and parses it into the passed existing attributes.
   * @param attribs (out) an existing attributes to be modified.
   * @param jsonConfig json document to parse
   * @return true if tag is found, of appropriate configuration, and holds
   * actual values.
   */
  bool parseUserDefinedJsonVals(
      const attributes::AbstractAttributes::ptr& attribs,
      const io::JsonGenericValue& jsonConfig) const;

 protected:
  /**
   * @brief Saves @p attributes::AbstractAttributes to a JSON file using the
   * given @p fileName in the given @p fileDirectory .
   * @param attribs The name of the object to save. If not found, returns
   * false.
   * @param filename The filename of the file to save to.
   * @param fileDirectory The directory to save to. If the directory does not
   * exist, will return false.
   * @return Whether save was successful
   */
  bool saveManagedObjectToFileInternal(
      const AttribsPtr& attribs,
      const std::string& filename,
      const std::string& fileDirectory) const override;

  /**
   * @brief Called intenrally from createObject.  This will create either a
   * file based AbstractAttributes or a default one based on whether the
   * passed file name exists and has appropriate string tag/extension for @ref
   * esp::metadata::attributes::AbstractAttributes.
   *
   * @param filename the file holding the configuration of the object
   * @param msg reference to progress message
   * @param registerObj whether the new object should be registered in library
   * @return the create @ref esp::metadata::attributes::AbstractAttributes.
   */
  AttribsPtr createFromJsonOrDefaultInternal(const std::string& filename,
                                             std::string& msg,
                                             bool registerObj);

 public:
  ESP_SMART_POINTERS(AttributesManager<T, Access>)

};  // class AttributesManager

/////////////////////////////
// Class Template Method Definitions
template <class T, ManagedObjectAccess Access>
std::vector<int> AttributesManager<T, Access>::loadAllFileBasedTemplates(
    const std::vector<std::string>& paths,
    bool saveAsDefaults) {
  std::vector<int> templateIndices(paths.size(), ID_UNDEFINED);
  if (paths.size() > 0) {
    std::string dir = Cr::Utility::Directory::path(paths[0]);
    ESP_DEBUG() << "Loading" << paths.size() << "" << this->objectType_
                << "templates found in" << dir;
    for (int i = 0; i < paths.size(); ++i) {
      auto attributesFilename = paths[i];
      ESP_VERY_VERBOSE() << "Load" << this->objectType_ << "template:"
                         << Cr::Utility::Directory::filename(
                                attributesFilename);
      auto tmplt = this->createObject(attributesFilename, true);
      // If failed to load, do not attempt to modify further
      if (tmplt == nullptr) {
        continue;
      }
      // save handles in list of defaults, so they are not removed, if desired.
      if (saveAsDefaults) {
        std::string tmpltHandle = tmplt->getHandle();
        this->undeletableObjectNames_.insert(tmpltHandle);
      }
      templateIndices[i] = tmplt->getID();
    }
  }
  ESP_DEBUG() << "<" << Magnum::Debug::nospace << this->objectType_
              << Magnum::Debug::nospace << "> : Loaded file-based templates:"
              << std::to_string(paths.size());
  return templateIndices;
}  // AttributesManager<T, Access>::loadAllObjectTemplates

template <class T, ManagedObjectAccess Access>
std::vector<int> AttributesManager<T, Access>::loadAllTemplatesFromPathAndExt(
    const std::string& path,
    const std::string& extType,
    bool saveAsDefaults) {
  namespace Dir = Cr::Utility::Directory;
  std::vector<std::string> paths;
  std::vector<int> templateIndices;

  // Check if directory
  const bool dirExists = Dir::isDirectory(path);
  if (dirExists) {
    ESP_DEBUG() << "Parsing" << this->objectType_
                << "library directory: " + path + " for \'" + extType +
                       "\' files";
    for (auto& file : Dir::list(path, Dir::Flag::SortAscending)) {
      std::string absoluteSubfilePath = Dir::join(path, file);
      if (Cr::Utility::String::endsWith(absoluteSubfilePath, extType)) {
        paths.push_back(absoluteSubfilePath);
      }
    }
  } else {
    // not a directory, perhaps a file
    std::string attributesFilepath =
        this->convertFilenameToPassedExt(path, extType);
    const bool fileExists = Dir::exists(attributesFilepath);

    if (fileExists) {
      paths.push_back(attributesFilepath);
    } else {  // neither a directory or a file
      ESP_WARNING() << "<" << Magnum::Debug::nospace << this->objectType_
                    << Magnum::Debug::nospace << "> : Parsing"
                    << this->objectType_ << ": Cannot find" << path
                    << "as directory or" << attributesFilepath
                    << "as config file. Aborting parse.";
      return templateIndices;
    }  // if fileExists else
  }    // if dirExists else

  // build templates from aggregated paths
  templateIndices = this->loadAllFileBasedTemplates(paths, saveAsDefaults);

  return templateIndices;
}  // AttributesManager<T, Access>::loadAllTemplatesFromPathAndExt

template <class T, ManagedObjectAccess Access>
void AttributesManager<T, Access>::buildAttrSrcPathsFromJSONAndLoad(
    const std::string& configDir,
    const std::string& extType,
    const io::JsonGenericValue& filePaths) {
  for (rapidjson::SizeType i = 0; i < filePaths.Size(); ++i) {
    if (!filePaths[i].IsString()) {
      ESP_ERROR() << "Invalid path value in file path array element @ idx" << i
                  << ". Skipping.";
      continue;
    }
    std::string absolutePath =
        Cr::Utility::Directory::join(configDir, filePaths[i].GetString());
    std::vector<std::string> globPaths = io::globDirs(absolutePath);
    if (globPaths.size() > 0) {
      for (const auto& globPath : globPaths) {
        // load all object templates available as configs in absolutePath
        ESP_WARNING() << "Glob path result for" << absolutePath << ":"
                      << globPath;
        this->loadAllTemplatesFromPathAndExt(globPath, extType, true);
      }
    } else {
      ESP_WARNING() << "No Glob path result for" << absolutePath;
    }
  }
  ESP_DEBUG() << "<" << Magnum::Debug::nospace << this->objectType_
              << Magnum::Debug::nospace
              << ">:" << std::to_string(filePaths.Size())
              << "paths specified in JSON doc for" << this->objectType_
              << "templates.";
}  // AttributesManager<T, Access>::buildAttrSrcPathsFromJSONAndLoad

template <class T, ManagedObjectAccess Access>
auto AttributesManager<T, Access>::createFromJsonOrDefaultInternal(
    const std::string& filename,
    std::string& msg,
    bool registerObj) -> AttribsPtr {
  AttribsPtr attrs;
  // Modify the passed filename to have the format of a legitimate
  // configuration file for this Attributes by changing the extension
  std::string jsonAttrFileName =
      (Cr::Utility::String::endsWith(filename, this->JSONTypeExt_)
           ? filename
           : this->getFormattedJSONFileName(filename));
  // Check if this configuration file exists and if so use it to build
  // attributes
  bool jsonFileExists = Cr::Utility::Directory::exists(jsonAttrFileName);
  ESP_DEBUG() << "<" << Magnum::Debug::nospace << this->objectType_
              << Magnum::Debug::nospace
              << ">: Proposing JSON name :" << jsonAttrFileName
              << "from original name :" << filename << "| This file"
              << (jsonFileExists ? " exists." : " does not exist.");
  if (jsonFileExists) {
    // configuration file exists with requested name, use to build Attributes
    attrs = this->createObjectFromJSONFile(jsonAttrFileName, registerObj);
    msg = "JSON Configuration File (" + jsonAttrFileName + ") based";
  } else {
    // An existing, valid configuration file could not be found using the
    // passed filename. Currently non-JSON filenames are used to create new,
    // default attributes.
    attrs = this->createDefaultObject(filename, registerObj);
    // check if original filename is an actual object
    bool fileExists = Cr::Utility::Directory::exists(filename);
    // if filename passed is name of some kind of asset, or if it was not
    // found
    if (fileExists) {
      msg = "File (" + filename +
            ") exists but is not a recognized config filename extension, so "
            "new default";
    } else {
      msg = "File (" + filename + ") not found, so new default";
    }
  }
  return attrs;
}  // AttributesManager<T, Access>::createFromJsonFileOrDefaultInternal

template <class T, ManagedObjectAccess Access>
bool AttributesManager<T, Access>::parseUserDefinedJsonVals(
    const attributes::AbstractAttributes::ptr& attribs,
    const io::JsonGenericValue& jsonConfig) const {
  // check for user defined attributes and verify it is an object
  if (jsonConfig.HasMember("user_defined")) {
    if (!jsonConfig["user_defined"].IsObject()) {
      ESP_WARNING() << "<" << Magnum::Debug::nospace << this->objectType_
                    << Magnum::Debug::nospace
                    << "> :" << attribs->getSimplifiedHandle()
                    << "attributes specifies user_defined attributes but they "
                       "are not of the correct format. Skipping.";
      return false;
    } else {
      const std::string subGroupName = "user_defined";
      // get pointer to user_defined subgroup configuration
      std::shared_ptr<Configuration> subGroupPtr =
          attribs->getUserConfiguration();
      // get json object referenced by tag subGroupName
      const io::JsonGenericValue& jsonObj = jsonConfig[subGroupName.c_str()];

      // count number of valid user config settings found
      int numConfigSettings = subGroupPtr->loadFromJson(jsonObj);

      // save as user_defined subgroup configuration
      attribs->setSubconfigPtr("user_defined", subGroupPtr);

      return (numConfigSettings > 0);
    }
  }  // if has user_defined tag
  return false;
}  // AttributesManager<T, Access>::parseUserDefinedJsonVals

template <class T, ManagedObjectAccess Access>
bool AttributesManager<T, Access>::saveManagedObjectToFileInternal(
    const AttribsPtr& attribs,
    const std::string& filename,
    const std::string& fileDirectory) const {
  namespace Dir = Cr::Utility::Directory;
  if (!Dir::exists(fileDirectory)) {
    // output directory not found
    ESP_ERROR() << "<" << this->objectType_ << "> : Destination directory "
                << fileDirectory << " does not exist to save "
                << attribs->getSimplifiedHandle() << "object. Aborting.";
    return false;
  }
  // construct fully qualified filename
  std::string fullFilename = Dir::join(fileDirectory, filename);
  ESP_DEBUG() << "Attempting to write file" << fullFilename << "to disk";
  // write configuration to file

  rapidjson::Document doc(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  // build Json from passed Configuration
  auto configJson = attribs->writeToJsonValue(allocator);
  // move constructed config into doc
  doc.Swap(configJson);
  // save to file
  bool success = io::writeJsonToFile(doc, fullFilename, true, 7);

  // bool success = io::writeConfigurationToJsonFile(fullFilename, attribs);
  ESP_DEBUG() << "Attempt to write file" << fullFilename
              << "to disk :" << (success ? "Successful" : "Failed");

  return success;

}  // AttributesManager<T, Access>::saveManagedObjectToFileInternal

}  // namespace managers
}  // namespace metadata
}  // namespace esp
#endif  // ESP_METADATA_MANAGERS_ATTRIBUTESMANAGERBASE_H_
