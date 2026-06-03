---
name: release
description: Guide for performing the DirectX Tool Kit for DirectX 11 release process. Use this skill when asked to help with releasing a new version, publishing packages, or updating ports.
---

# Release Process

## Prerequisites

- All changes merged into the `main` branch with all tests passing.
- GPG signing configured for your GitHub account (for verified tags).
- Access to the MSCodeHub mirror repository and Azure DevOps pipelines.
- Local repositories:
  - VCPKG at `d:\vcpkg` (synced with `main` branch)
  - WinGet at `D:\winget-pkgs` (synced with `master` branch, only if tool updates needed)
- PATs will be needed for scripts that access GitHub and ADO.

<!-- markdownlint-disable MD029 -->
## Steps

### Phase 1: Prepare Release

1. Git pull the local repository to ensure it is up to date with the `main` branch.
2. Run the PowerShell script `build\preparerelease.ps1` which will generate a topic branch for the release, update the version number in `CMakeLists.txt`, the `README.md` file, the release notes in the nuspec files, and create a stub in the `CHANGELOG.md` file for the new release.
3. Edit the `CHANGELOG.md` file to update it with a summary of changes.
4. Submit the topic branch for review and merge into `main` once approved. Allow the GitHub Actions workflows and the Azure DevOps pipelines to complete successfully before proceeding.

### Phase 2: Tag and Create GitHub Release

5. Run the PowerShell script `build\completerelease.ps1` which will set a tag on the project repo and the test repo, and create a release on GitHub with the release notes from `CHANGELOG.md`. Ensure you have set up GPG signing for your GitHub account so that the tags will be verified.
6. Git pull the local repository to ensure it is up to date with the `main` branch. Be sure to include `--tags`.

### Phase 3: MSCodeHub and Signed Binaries

7. Push the `main` branch to the MSCodeHub mirror repository. Be sure to include `--tags`.
8. Create a PR on MSCodeHub from the `main` branch to the `release` branch.
9. Merge the PR on MSCodeHub to update the release branch, which will trigger the Azure DevOps pipeline to build signed binaries and the NuGet packages.
10. Run the PowerShell script `build\downloadbuild.ps1` to download the signed binaries from the Azure DevOps pipeline artifacts.
11. Edit the GitHub release and upload the signed binaries to the release assets.

### Phase 4: Source Archive Signing

12. Download the GitHub source .zip archive from the release. Unzip and compare to the local repo to ensure it matches — keep in mind there may be some CR/LF differences.
13. Run minisign on the .zip to generate a signature file, and upload the signature file to the release assets.

### Phase 5: NuGet Validation and Publishing

14. Validate the NuGet packages with <https://github.com/walbourn/directxtk-tutorials> by pushing the NuGet packages to a local Packages Source folder, and refreshing the NuGet packages from that folder. Then build using BuildAllSolutions.targets.
15. Run the PowerShell script `build\promotenuget.ps1 -Version <version> -Release` to promote the version to the Release view on the project-scoped ADO feed. The `-Version` parameter is required and should match the NuGet package version (e.g., `2026.6.2.1`).
16. Run the MSCodeHub pipeline to publish the NuGet packages to nuget.org. The pipeline will automatically push the most recent package promoted to the Release view to nuget.org.

### Phase 6: VCPKG Port Update

17. Git pull a local repository of VCPKG to `d:\vcpkg` in sync with the `main` branch of the VCPKG repository.
18. Run the PowerShell script `build\updatevcpkg.ps1` to update the DirectXTK port in VCPKG with the new release version. This will edit the files in `ports\directxtk`.
    If the port includes patches, review them to determine if they should be removed or updated for the new release (the `updatevcpkg.ps1` script will warn about this).
19. Test the VCPKG port using the script at `assets/vcpkgdxtk.cmd` (in this skill folder). Copy it to `d:\vcpkg` and run from there after bootstrapping VCPKG.
20. Run `.\vcpkg x-add-version directxtk` to update the VCPKG versioning history.
21. Submit a PR to the VCPKG repository to update the DirectXTK port back to the main GitHub repo. The PR will be reviewed and merged by the VCPKG maintainers.

### Phase 7: WinGet Manifests (Conditional)

If relevant changes were made to the `xwbtool` or `MakeSpriteFont` tools:

22. Git pull a local repository to `D:\winget-pkgs` in sync with the `master` branch of the WinGet repository.
23. Run the PowerShell script `build\updatewinget.ps1` to update the winget manifests for the tools with the new release version.
24. Submit a PR to the `winget` repository to update the manifests for each tool — they must be done as distinct PRs. The PRs will be reviewed and merged by the winget maintainers.

### Phase 8: Finalize

When fully completed, be sure to update the GitHub release with links to the matching NuGet packages, the VCPKG port, and the winget manifests for the tools.

## Key Scripts

| Script | Purpose |
| --- | --- |
| `build\preparerelease.ps1` | Creates topic branch, updates version numbers and changelog stub |
| `build\completerelease.ps1` | Sets tags, creates GitHub release from changelog |
| `build\downloadbuild.ps1` | Downloads signed binaries from Azure DevOps |
| `build\promotenuget.ps1 -Release` | Promotes NuGet package to Release view on ADO feed |
| `build\updatevcpkg.ps1` | Updates DirectXTK VCPKG port files |
| `assets\vcpkgdxtk.cmd` | Tests VCPKG port across all triplets and features |
| `build\updatewinget.ps1` | Updates winget manifests for CLI tools |
