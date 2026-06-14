import os

Import("env")

if env.IsIntegrationDump():
    # stop the current script execution
    Return()

src = os.path.join("$PROJECT_SRC_DIR", "video.avi")
dest = os.path.join("$PROJECT_SRC_DIR", "video_avi.c")
env.Execute(f"xxd -i -n video_avi {src} {dest}")
env.BuildSources(src, dest)
