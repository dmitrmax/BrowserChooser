cmake_minimum_required(VERSION 3.16)
project(BrowserChooser VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Qt auto processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Prefer Qt6, fallback to Qt5
find_package(Qt6 COMPONENTS Widgets Core Gui LinguistTools REQUIRED)
set(QT_PACKAGE Qt6)
qt_standard_project_setup(I18N_SOURCE_LANGUAGE en I18N_TRANSLATED_LANGUAGES de_DE ru_RU)

# ---- Translations ----
# file(GLOB TRANSLATION_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/*.ts")

# Генерация .qm через qt_add_translations
# qt_add_translations(BrowserChooser TS_FILE_DIR "translations")

# ---- Ресурсы ----
qt_add_resources(APP_RESOURCES resources/app.qrc)

add_executable(BrowserChooser
  src/main.cpp
  src/BrowserConfig.h src/BrowserConfig.cpp
  src/BrowserManager.h src/BrowserManager.cpp
  src/ChoiceStore.h src/ChoiceStore.cpp
  src/ConfigDialog.h src/ConfigDialog.cpp
  src/MainWindow.h src/MainWindow.cpp
  src/ChoiceEditorDialog.h src/ChoiceEditorDialog.cpp
  src/UrlUtils.h src/UrlUtils.cpp
  ${APP_RESOURCES}
)

target_link_libraries(BrowserChooser PRIVATE ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Core ${QT_PACKAGE}::Gui)

# macOS: Objective-C++ bridge for default handler
if(APPLE)
  set_source_files_properties(src/MacDefault.mm PROPERTIES
      COMPILE_FLAGS "-fobjc-arc")
  target_sources(BrowserChooser PRIVATE src/MacDefault.mm)
  target_link_libraries(BrowserChooser PRIVATE "-framework CoreServices" "-framework Foundation")
  set_target_properties(BrowserChooser PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.example.BrowserChooser"
    MACOSX_BUNDLE_BUNDLE_NAME "BrowserChooser"
  )
endif()

