from marlin import copytree
def debug_mv(*args, **kwargs):
	from pathlib import Path
	bpath = Path(env['PROJECT_BUILD_DIR'], env['PIOENV'])
	copytree(bpath / "debug", bpath)

env.AddCustomTarget("fastdebug", "$BUILD_DIR/${PROGNAME}", debug_mv, "Fast Debug")