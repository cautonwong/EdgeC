This project is only used to run on core0.

If use IDE, just read the 工具链使用说明.md or 工具链使用说明.pdf(DIR: PD-FM-DEV_KIT_9701C\A1_Design\B1_Scheme\)
If use CMD, steps are as follows:
	1) Check the nuclei_studio(latest version:2025.02) installation directory first,
	2) If IDE is not in 'D:\NucleiStudio\toolchain', then you need to modify setup.bat(DIR: nuclei_sdk\),
	3) Run command in CMD: 
			a. '.\setup.bat'
			b. 'cd application\renergy_demo\helloworld'
			c. 'make -j20 all'
			d. 'make run_openocd'(in terminal A)
			e. 'make run_gdb'(in terminal B, if need to debug)
	4) If the programs ends, you can run command: 'make clean' to clear the helloworld.xxx.
	
We recommend using a project-based relative path like "${ProjDirPath}", and not a workspace-base relative path like "${workspace_loc:/}".