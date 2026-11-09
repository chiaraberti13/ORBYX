// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <opencv2/core.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <array>
namespace orbyx {
// Scene coordinates are source-resolution pixels. Order is bottom to top.
struct MosaicLayer {
    std::filesystem::path image;
    double x=0, y=0, scale=1, angle=0, opacity=1, feather=0;
    cv::Rect crop; // empty means the full source
    double black=0, white=1, gamma=1, exposure=0, saturation=1;
    double red=1, green=1, blue=1;
    bool visible=true;
};
struct MosaicResult {
    std::vector<std::array<cv::Point2d,4>> outlines;
    cv::Mat image; // native BGR 8/16-bit; preview is explicitly 8-bit
    double originX=0, originY=0, pixelsPerSourcePixel=1;
};
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
std::string mosaicProject(const std::vector<MosaicLayer>&);
std::vector<MosaicLayer> parseMosaicProject(const std::string&);
}
