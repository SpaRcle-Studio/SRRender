set(SR_RENDER_WEB_GPU_LIB_DIR "${SR_CMAKE_ROOT_SOURCE_DIRECTORY}/Platform/Emscripten/WebGPU")
set(SR_RENDER_WEB_GPU_LIB_ARCHIVE "${SR_CMAKE_ROOT_SOURCE_DIRECTORY}/Platform/Emscripten/WebGPU.tar.gz")

#set(SR_RENDER_WEB_GPU_WINDOWS_DEBUG_ARCHIVE_URL "https://release-assets.githubusercontent.com/github-production-release-asset/700471558/2eafc42d-380f-4c52-96d5-8d346d1ce708?sp=r&sv=2018-11-09&sr=b&spr=https&se=2026-03-01T10%3A59%3A49Z&rscd=attachment%3B+filename%3DDawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Debug.tar.gz&rsct=application%2Foctet-stream&skoid=96c2d410-5711-43a1-aedd-ab1947aa7ab0&sktid=398a6654-997b-47e9-b12b-9515b896b4de&skt=2026-03-01T09%3A59%3A06Z&ske=2026-03-01T10%3A59%3A49Z&sks=b&skv=2018-11-09&sig=1w%2BoBYdGkANOa6jaPkupAQrDxFgW7Rk1heFmxNmYoGI%3D&jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmVsZWFzZS1hc3NldHMuZ2l0aHVidXNlcmNvbnRlbnQuY29tIiwia2V5Ijoia2V5MSIsImV4cCI6MTc3MjM2Mzk2MywibmJmIjoxNzcyMzYwMzYzLCJwYXRoIjoicmVsZWFzZWFzc2V0cHJvZHVjdGlvbi5ibG9iLmNvcmUud2luZG93cy5uZXQifQ.jlgh23hLZWGBtNCqUpjxMukmF2FT9cGGZ4iEyslVvVY&response-content-disposition=attachment%3B%20filename%3DDawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Debug.tar.gz&response-content-type=application%2Foctet-stream")
#set(SR_RENDER_WEB_GPU_WINDOWS_RELEASE_ARCHIVE_URL "https://release-assets.githubusercontent.com/github-production-release-asset/700471558/c1c96d3e-64de-4dd4-9346-a013bf000dc6?sp=r&sv=2018-11-09&sr=b&spr=https&se=2026-03-01T11%3A14%3A48Z&rscd=attachment%3B+filename%3DDawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Release.tar.gz&rsct=application%2Foctet-stream&skoid=96c2d410-5711-43a1-aedd-ab1947aa7ab0&sktid=398a6654-997b-47e9-b12b-9515b896b4de&skt=2026-03-01T10%3A14%3A05Z&ske=2026-03-01T11%3A14%3A48Z&sks=b&skv=2018-11-09&sig=Om1I3LZO%2B4jR8hjQKK2nERZKJzvAtP86mDUI8pshKeA%3D&jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmVsZWFzZS1hc3NldHMuZ2l0aHVidXNlcmNvbnRlbnQuY29tIiwia2V5Ijoia2V5MSIsImV4cCI6MTc3MjM2MjE4NiwibmJmIjoxNzcyMzYwMzg2LCJwYXRoIjoicmVsZWFzZWFzc2V0cHJvZHVjdGlvbi5ibG9iLmNvcmUud2luZG93cy5uZXQifQ.c__vE1S31hzT5YUQcdwKbL7IDssOVap7xLO6_xzsX4I&response-content-disposition=attachment%3B%20filename%3DDawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Release.tar.gz&response-content-type=application%2Foctet-stream")

set(SR_RENDER_WEB_GPU_WINDOWS_DEBUG_ARCHIVE_URL "https://github.com/google/dawn/releases/download/v20260218.195015/Dawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Debug.tar.gz")
set(SR_RENDER_WEB_GPU_WINDOWS_RELEASE_ARCHIVE_URL "https://github.com/google/dawn/releases/download/v20260218.195015/Dawn-6d3c8e9fd2a674a336f33b91977bb353b78510ee-windows-latest-Release.tar.gz")

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SR_RENDER_WEB_GPU_ARCHIVE_URL ${SR_RENDER_WEB_GPU_WINDOWS_DEBUG_ARCHIVE_URL})
    else()
        set(SR_RENDER_WEB_GPU_ARCHIVE_URL ${SR_RENDER_WEB_GPU_WINDOWS_RELEASE_ARCHIVE_URL})
    endif()
else()
    message(FATAL_ERROR "WebGPU prebuilt binaries are only available for Windows. Please build WebGPU from source for other platforms.")
endif()

if (NOT EXISTS "${SR_RENDER_WEB_GPU_LIB_DIR}")
    message(STATUS "WebGPU ${SR_RENDER_WEB_GPU_LIB_DIR} not installed. Install from ${SR_RENDER_WEB_GPU_ARCHIVE_URL}...")

    execute_process(
        COMMAND curl -L -o "${SR_RENDER_WEB_GPU_LIB_ARCHIVE}" "${SR_RENDER_WEB_GPU_ARCHIVE_URL}"
        RESULT_VARIABLE CURL_RESULT
        ERROR_VARIABLE CURL_ERROR
    )

    if (NOT CURL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download WebGPU archive from ${SR_RENDER_WEB_GPU_ARCHIVE_URL}. Status: ${DOWNLOAD_STATUS}")
    endif()

    message(STATUS "Unpacking WebGPU archive...")
    file(ARCHIVE_EXTRACT
        INPUT ${SR_RENDER_WEB_GPU_LIB_ARCHIVE}
        DESTINATION ${SR_RENDER_WEB_GPU_LIB_DIR}
    )

    file(GLOB HASH_DIRS "${SR_RENDER_WEB_GPU_LIB_DIR}/Dawn-*")
    if (HASH_DIRS)
        message(STATUS "Moving WebGPU files to the correct location...")
        set(HASH_DIR "${HASH_DIRS}")  # берём первую (обычно одна)
        file(GLOB CHILDREN "${HASH_DIR}/*")
        foreach(CHILD ${CHILDREN})
            get_filename_component(NAME "${CHILD}" NAME)  # имя файла/папки
            file(RENAME "${CHILD}" "${SR_RENDER_WEB_GPU_LIB_DIR}/${NAME}")
        endforeach()

        message(STATUS "Deleting temporary WebGPU folder...")
        file(REMOVE_RECURSE "${HASH_DIR}")
    else()
        message(FATAL_ERROR "Failed to find the extracted WebGPU files in ${SR_RENDER_WEB_GPU_LIB_DIR}. Expected a folder starting with 'Dawn-'.")
    endif()

    file(REMOVE "${SR_RENDER_WEB_GPU_LIB_ARCHIVE}")

    message(STATUS "WebGPU installed successfully!")
else()
    message(STATUS "WebGPU ${SR_RENDER_WEB_GPU_LIB_DIR} already exists.")
endif()