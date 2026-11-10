// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <opencv2/core.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <array>
namespace orbyx {
enum class MosaicBlend {Replace,Feather,Seam,Multiband};
struct MosaicMask {
    bool include=false; // Include masks form a union; exclude masks override it.
    std::vector<cv::Point2d> points; // Coordinates inside the cropped layer.
};
// Scene coordinates are source-resolution pixels. Order is bottom to top.
struct MosaicLayer {
    std::filesystem::path image;
    double x=0, y=0, scale=1, angle=0, opacity=1, feather=0;
    cv::Rect crop; // empty means the full source
    cv::Rect exclude; // empty means no mask; otherwise this region of the (cropped) layer does not contribute, revealing layers below. Non-destructive: source pixels are untouched.
    std::vector<MosaicMask> masks; // Optional non-destructive polygon masks.
    double black=0, white=1, gamma=1, exposure=0, saturation=1;
    double red=1, green=1, blue=1;
    MosaicBlend blend=MosaicBlend::Feather;
    bool visible=true;
};
struct MosaicResult {
    std::vector<std::array<cv::Point2d,4>> outlines;
    cv::Mat image; // native BGR 8/16-bit; preview is explicitly 8-bit
    double originX=0, originY=0, pixelsPerSourcePixel=1;
    int seamFallbacks=0; // Seam layers that used the deterministic feather fallback.
};
// Quality indicators for the current composition (M2-12). Evidence only: it never
// declares an image "perfect" and the caller decides. All fractions are 0..1.
struct MosaicQuality {
    std::size_t coveredPixels=0;   // Pixels covered by at least one layer (preview).
    double emptyFraction=0;        // Uncovered pixels inside the coverage bounding box (gaps/holes).
    double overlapFraction=0;      // Covered pixels under two or more layers.
    double highlightClipped=0;     // Covered pixels with a channel at the maximum.
    double shadowClipped=0;        // Covered pixels that are pure black.
    int extremeTransforms=0;       // Visible layers rotated >=45 deg or scaled outside [0.5, 2].
};
// Bounded, deterministic quality assessment rendered at a preview edge (64..2048).
MosaicQuality mosaicQuality(const std::vector<MosaicLayer>& layers,int previewEdge=1024);
struct OverlapMatch {
    std::size_t moving=0, reference=0;
    int candidateMatches=0, inliers=0;
    double confidence=0, medianError=0, overlapFraction=0;
};
struct AutoOverlapResult {
    std::vector<MosaicLayer> layers;
    std::vector<OverlapMatch> accepted;
    std::vector<std::size_t> unaligned;
};
struct ControlPointPair {
    cv::Point2d moving;    // Coordinates inside the moving cropped portion.
    cv::Point2d reference; // Coordinates inside the reference cropped portion.
};
struct ControlPointSolution {
    MosaicLayer layer;
    std::vector<double> residuals;       // One value per supplied pair.
    std::vector<std::size_t> inliers;    // Robustly accepted pair indices.
    double medianError=0, maxError=0;    // Inlier residuals, in reference pixels.
    double movingCoverage=0, referenceCoverage=0; // Inlier bounding-box fraction.
    bool spatiallyConcentrated=false;    // Warning only; never rejects a valid fit.
};
struct CropProposal {
    cv::Rect original;
    cv::Rect proposed; // Absolute source-image coordinates.
    double retainedFraction=1;
};
struct VisualCompensation {
    MosaicLayer layer; // Proposed moving layer; input layers remain unchanged.
    std::size_t samples=0;
    double exposureDelta=0;
    double redMultiplier=1, greenMultiplier=1, blueMultiplier=1;
};
// Strict, bounded PNG reader shared with the core. Grayscale or RGB 8/16-bit.
cv::Mat readCompositePng(const std::filesystem::path&);
void writeCompositePng(const cv::Mat&,const std::filesystem::path&);
// previewEdge=0 exports full resolution; a positive edge renders a bounded preview.
MosaicResult renderMosaic(const std::vector<MosaicLayer>&,int previewEdge=0);
// Find genuine visual overlap and propose editable similarity transforms. The
// selected anchor is never changed. Portions without a sufficiently reliable
// match remain untouched and are listed in unaligned. No geographic meaning is
// inferred, no pixels are written and the caller decides whether to apply.
AutoOverlapResult autoAlignOverlaps(const std::vector<MosaicLayer>&,std::size_t anchor=0);
// Fit a similarity transform from user-reviewed point pairs. The returned
// moving layer is placed in the reference layer's scene coordinate system.
// At least three finite, non-degenerate pairs are required; RANSAC rejects
// outliers and residuals are reported for every supplied pair.
ControlPointSolution alignFromControlPoints(const MosaicLayer& moving,const MosaicLayer& reference,
                                            const std::vector<ControlPointPair>& pairs);
// Detect low-detail outer rows/columns and return a reversible crop proposal.
// No layer or source pixel is modified. Uniform/ambiguous images are rejected.
CropProposal proposeUniformBorderCrop(const MosaicLayer& layer);
// Estimate a robust, optional visual match from the genuine overlap. This only
// proposes exposure/RGB controls already used by the compositor; the caller
// must explicitly apply it. It is not radiometric calibration.
VisualCompensation proposeVisualCompensation(const std::vector<MosaicLayer>&,
                                             std::size_t moving,std::size_t reference);
std::string mosaicProject(const std::vector<MosaicLayer>&);
std::vector<MosaicLayer> parseMosaicProject(const std::string&);
// Deterministic export report (M2-11): declares the output bit depth, a colour
// note (visual rendering, not radiometric calibration) and the full reproducible
// project of non-destructive steps. bitDepth must be 8 or 16.
std::string mosaicReport(const std::vector<MosaicLayer>& layers,const std::string& output,int bitDepth);
}
