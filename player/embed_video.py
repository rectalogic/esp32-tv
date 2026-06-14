import os

Import("env")

if env.IsIntegrationDump():
    # stop the current script execution
    Return()

src = os.path.join("$PROJECT_SRC_DIR", "video.avi")
dest = os.path.join("$PROJECT_SRC_DIR", "video_avi.embed")
env.Execute(
    f"/usr/bin/xxd -i -n video_avi {src} | /usr/bin/sed 's/^unsigned char/static const unsigned char/' > {dest}"
)
env.BuildSources(src, dest)
