# Description
Some offensive C++ Windows tools that utilize [OffWinLib](https://github.com/SpacePlant/OffWinLib).

# Projects Overview
- **Symlink**: Sample tool for creating symbolic file system links ([based on this](https://github.com/googleprojectzero/symboliclink-testing-tools/tree/main/CreateSymlink)).
- **Reglink**: Sample tool for creating and manipulating registry links ([based on this](https://github.com/googleprojectzero/symboliclink-testing-tools/tree/main/CreateRegSymlink)).
- **Oplock**: Sample tool for using opportunistic locks ([based on this](https://github.com/googleprojectzero/symboliclink-testing-tools/tree/main/SetOpLock)).
- **DLLInjector**: Sample tool for injecting DLLs into processes.
- **ShellcodeInjector**: Sample tool for injecting shellcode into processes.
- **FolderContentsDeleteToArbitraryDelete**: Sample tool for turning an arbitrary folder contents delete primitive into an arbitrary file or folder delete primitive ([based on this](https://www.zerodayinitiative.com/blog/2022/3/16/abusing-arbitrary-file-deletes-to-escalate-privilege-and-other-great-tricks)).
- **FolderDeleteToCodeExec**: Sample tool for turning an arbitrary folder delete primitive into privileged code execution ([also based on this](https://www.zerodayinitiative.com/blog/2022/3/16/abusing-arbitrary-file-deletes-to-escalate-privilege-and-other-great-tricks)).
- **ErrorMSI**: MSI installer package that throws an error and initiates a rollback immediately.

# Dependencies
- [OffWinLib](https://github.com/SpacePlant/OffWinLib) is used as a Git submobule.
- The FolderDeleteToCodeExec project requires [Windows Implementation Libraries (WIL)](https://github.com/microsoft/wil) via the Microsoft.Windows.ImplementationLibrary NuGet package. If it is not installed automatically on build, run `nuget restore`.
- The ErrorMSI project requires the [HeatWave for VS2022](https://marketplace.visualstudio.com/items?itemName=FireGiant.FireGiantHeatWaveDev17) Visual Studio extension.

# How to Build
1. Clone the repository with `git clone --recursive`.
2. Build the OffWinLib project from the OffWinLib solution to ensure that dependencies are installed properly.
3. Build the OffWinSamples solution.
