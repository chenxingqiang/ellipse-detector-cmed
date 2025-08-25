/**
 * CMED Ellipse Detection Evaluator (C++ Implementation)
 * ====================================================
 * 
 * A comprehensive evaluation framework for the Characteristic Mapping for 
 * Ellipse Detection (CMED) algorithm.
 * 
 * This module provides tools to evaluate ellipse detection performance using
 * various metrics including F-measure, precision, recall, and overlap ratios.
 * 
 * Author: Chen Xingqiang
 * Date: 2024
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Structure to hold evaluation metrics for a single image
 */
struct EvaluationMetrics {
    double precision;
    double recall;
    double f_measure;
    int total_gt;
    int total_detected;
    int true_positives;
    int false_positives;
    int false_negatives;
    double average_overlap;
    double processing_time;
    
    EvaluationMetrics() : precision(0.0), recall(0.0), f_measure(0.0),
                         total_gt(0), total_detected(0), true_positives(0),
                         false_positives(0), false_negatives(0),
                         average_overlap(0.0), processing_time(0.0) {}
};

/**
 * Structure to hold ellipse parameters
 */
struct EllipseParams {
    double a;      // semi-major axis
    double b;      // semi-minor axis
    double x;      // center x coordinate
    double y;      // center y coordinate
    double theta;  // rotation angle in radians
    
    EllipseParams() : a(0), b(0), x(0), y(0), theta(0) {}
    EllipseParams(double a, double b, double x, double y, double theta)
        : a(a), b(b), x(x), y(y), theta(theta) {}
};

/**
 * Ellipse Overlap Calculator Class
 * 
 * Calculates overlap ratios between ellipses for evaluation purposes.
 */
class EllipseOverlapCalculator {
private:
    cv::Size image_size_;
    cv::Mat pixels_x_;
    cv::Mat pixels_y_;
    
public:
    /**
     * Constructor
     * @param image_size Size of the image (width, height)
     */
    EllipseOverlapCalculator(const cv::Size& image_size) : image_size_(image_size) {
        createCoordinateGrid();
    }
    
    /**
     * Create coordinate grids for pixel-wise calculations
     */
    void createCoordinateGrid() {
        pixels_x_ = cv::Mat(image_size_, CV_32F);
        pixels_y_ = cv::Mat(image_size_, CV_32F);
        
        for (int y = 0; y < image_size_.height; ++y) {
            for (int x = 0; x < image_size_.width; ++x) {
                pixels_x_.at<float>(y, x) = static_cast<float>(x + 1);
                pixels_y_.at<float>(y, x) = static_cast<float>(y + 1);
            }
        }
    }
    
    /**
     * Calculate binary mask for an ellipse
     * @param ellipse Ellipse parameters
     * @return Binary mask where true indicates pixels inside the ellipse
     */
    cv::Mat calculateEllipseMask(const EllipseParams& ellipse) const {
        cv::Mat mask = cv::Mat::zeros(image_size_, CV_8U);
        
        double sin_theta = std::sin(ellipse.theta);
        double cos_theta = std::cos(ellipse.theta);
        
        for (int y = 0; y < image_size_.height; ++y) {
            for (int x = 0; x < image_size_.width; ++x) {
                double dx = pixels_x_.at<float>(y, x) - ellipse.x;
                double dy = pixels_y_.at<float>(y, x) - ellipse.y;
                
                // Apply rotation and scaling
                double term1 = std::pow((dx * sin_theta - dy * cos_theta) / ellipse.b, 2);
                double term2 = std::pow((dx * cos_theta + dy * sin_theta) / ellipse.a, 2);
                
                // Ellipse equation: term1 + term2 <= 1
                if (term1 + term2 <= 1.0) {
                    mask.at<uchar>(y, x) = 255;
                }
            }
        }
        
        return mask;
    }
    
    /**
     * Calculate overlap ratio between two ellipses
     * @param ellipse1 Parameters of first ellipse
     * @param ellipse2 Parameters of second ellipse
     * @return Overlap ratio between 0 and 1
     */
    double calculateOverlapRatio(const EllipseParams& ellipse1, 
                                const EllipseParams& ellipse2) const {
        try {
            cv::Mat mask1 = calculateEllipseMask(ellipse1);
            cv::Mat mask2 = calculateEllipseMask(ellipse2);
            
            // Calculate XOR and OR operations
            cv::Mat xor_mask, or_mask;
            cv::bitwise_xor(mask1, mask2, xor_mask);
            cv::bitwise_or(mask1, mask2, or_mask);
            
            // Count pixels
            int xor_count = cv::countNonZero(xor_mask);
            int or_count = cv::countNonZero(or_mask);
            
            if (or_count == 0) {
                return 0.0;
            }
            
            return 1.0 - (static_cast<double>(xor_count) / or_count);
            
        } catch (const std::exception& e) {
            std::cerr << "Error calculating overlap ratio: " << e.what() << std::endl;
            return 0.0;
        }
    }
    
    /**
     * Calculate overlap matrix between ground truth and detected ellipses
     * @param ground_truth Vector of ground truth ellipses
     * @param detected Vector of detected ellipses
     * @return Overlap matrix
     */
    cv::Mat calculateOverlapMatrix(const std::vector<EllipseParams>& ground_truth,
                                  const std::vector<EllipseParams>& detected) const {
        int num_gt = ground_truth.size();
        int num_det = detected.size();
        
        cv::Mat overlap_matrix = cv::Mat::zeros(num_gt, num_det, CV_64F);
        
        for (int i = 0; i < num_gt; ++i) {
            for (int j = 0; j < num_det; ++j) {
                overlap_matrix.at<double>(i, j) = calculateOverlapRatio(
                    ground_truth[i], detected[j]);
            }
        }
        
        return overlap_matrix;
    }
};

/**
 * CMED Evaluator Class
 * 
 * Main evaluator class for CMED ellipse detection algorithm.
 */
class CMEDEvaluator {
private:
    double beta_threshold_;
    std::string evaluation_method_;
    
public:
    /**
     * Constructor
     * @param beta_threshold Overlap threshold for considering a detection as correct
     * @param evaluation_method 'A1' for Lu-Arc method, 'B2' for Jia-TIP method
     */
    CMEDEvaluator(double beta_threshold = 0.8, const std::string& evaluation_method = "A1")
        : beta_threshold_(beta_threshold), evaluation_method_(evaluation_method) {
        std::cout << "Initialized CMED Evaluator with beta=" << beta_threshold 
                  << ", method=" << evaluation_method << std::endl;
    }
    
    /**
     * Load ellipse data from text file
     * @param file_path Path to the ellipse data file
     * @return Vector of ellipse parameters
     */
    std::vector<EllipseParams> loadEllipseData(const std::string& file_path) const {
        std::vector<EllipseParams> ellipses;
        
        try {
            std::ifstream file(file_path);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file " << file_path << std::endl;
                return ellipses;
            }
            
            int num_ellipses;
            file >> num_ellipses;
            
            for (int i = 0; i < num_ellipses; ++i) {
                EllipseParams ellipse;
                file >> ellipse.a >> ellipse.b >> ellipse.x >> ellipse.y >> ellipse.theta;
                ellipses.push_back(ellipse);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading ellipse data from " << file_path << ": " << e.what() << std::endl;
        }
        
        return ellipses;
    }
    
    /**
     * Sort ellipse parameters for consistent comparison
     * @param ellipses Vector of ellipse parameters
     * @return Sorted ellipse parameters
     */
    std::vector<EllipseParams> sortEllipseParameters(const std::vector<EllipseParams>& ellipses) const {
        std::vector<EllipseParams> sorted = ellipses;
        
        // Sort by center coordinates (y first, then x)
        std::sort(sorted.begin(), sorted.end(), 
                 [](const EllipseParams& a, const EllipseParams& b) {
                     if (std::abs(a.y - b.y) < 1e-6) {
                         return a.x < b.x;
                     }
                     return a.y < b.y;
                 });
        
        return sorted;
    }
    
    /**
     * Evaluate ellipse detection for a single image
     * @param ground_truth_file Path to ground truth file
     * @param detected_file Path to detected ellipses file
     * @param image_size Image dimensions
     * @return Evaluation metrics for the image
     */
    EvaluationMetrics evaluateSingleImage(const std::string& ground_truth_file,
                                         const std::string& detected_file,
                                         const cv::Size& image_size) const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Load data
        std::vector<EllipseParams> ground_truth = loadEllipseData(ground_truth_file);
        std::vector<EllipseParams> detected = loadEllipseData(detected_file);
        
        // Sort parameters for consistent comparison
        ground_truth = sortEllipseParameters(ground_truth);
        detected = sortEllipseParameters(detected);
        
        EvaluationMetrics metrics;
        metrics.total_gt = ground_truth.size();
        metrics.total_detected = detected.size();
        
        // Handle empty cases
        if (ground_truth.empty() && detected.empty()) {
            metrics.precision = 1.0;
            metrics.recall = 1.0;
            metrics.f_measure = 1.0;
            metrics.average_overlap = 1.0;
            return metrics;
        }
        
        if (ground_truth.empty()) {
            metrics.precision = 0.0;
            metrics.recall = 1.0;
            metrics.f_measure = 0.0;
            metrics.false_positives = detected.size();
            return metrics;
        }
        
        if (detected.empty()) {
            metrics.precision = 1.0;
            metrics.recall = 0.0;
            metrics.f_measure = 0.0;
            metrics.false_negatives = ground_truth.size();
            return metrics;
        }
        
        // Calculate overlap matrix
        EllipseOverlapCalculator calculator(image_size);
        cv::Mat overlap_matrix = calculator.calculateOverlapMatrix(ground_truth, detected);
        
        // Calculate metrics based on evaluation method
        if (evaluation_method_ == "A1") {
            // Lu-Arc method: TP accumulation
            metrics.true_positives = 0;
            for (int i = 0; i < overlap_matrix.rows; ++i) {
                double max_overlap = 0.0;
                for (int j = 0; j < overlap_matrix.cols; ++j) {
                    max_overlap = std::max(max_overlap, overlap_matrix.at<double>(i, j));
                }
                if (max_overlap > beta_threshold_) {
                    metrics.true_positives++;
                }
            }
            metrics.false_negatives = ground_truth.size() - metrics.true_positives;
            
            // Count false positives
            int matched_detections = 0;
            for (int j = 0; j < overlap_matrix.cols; ++j) {
                double max_overlap = 0.0;
                for (int i = 0; i < overlap_matrix.rows; ++i) {
                    max_overlap = std::max(max_overlap, overlap_matrix.at<double>(i, j));
                }
                if (max_overlap > beta_threshold_) {
                    matched_detections++;
                }
            }
            metrics.false_positives = detected.size() - matched_detections;
            
        } else {  // B2 method
            // Jia-TIP method: per-image F-measure
            metrics.true_positives = 0;
            for (int i = 0; i < overlap_matrix.rows; ++i) {
                double max_overlap = 0.0;
                for (int j = 0; j < overlap_matrix.cols; ++j) {
                    max_overlap = std::max(max_overlap, overlap_matrix.at<double>(i, j));
                }
                if (max_overlap > beta_threshold_) {
                    metrics.true_positives++;
                }
            }
            metrics.false_negatives = ground_truth.size() - metrics.true_positives;
            metrics.false_positives = detected.size() - metrics.true_positives;
        }
        
        // Calculate precision, recall, and F-measure
        if (metrics.true_positives + metrics.false_positives > 0) {
            metrics.precision = static_cast<double>(metrics.true_positives) / 
                               (metrics.true_positives + metrics.false_positives);
        }
        
        if (metrics.true_positives + metrics.false_negatives > 0) {
            metrics.recall = static_cast<double>(metrics.true_positives) / 
                            (metrics.true_positives + metrics.false_negatives);
        }
        
        if (metrics.precision + metrics.recall > 0) {
            metrics.f_measure = 2.0 * metrics.precision * metrics.recall / 
                               (metrics.precision + metrics.recall);
        }
        
        // Calculate average overlap
        double total_overlap = 0.0;
        int overlap_count = 0;
        for (int i = 0; i < overlap_matrix.rows; ++i) {
            for (int j = 0; j < overlap_matrix.cols; ++j) {
                total_overlap += overlap_matrix.at<double>(i, j);
                overlap_count++;
            }
        }
        metrics.average_overlap = overlap_count > 0 ? total_overlap / overlap_count : 0.0;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        metrics.processing_time = duration.count() / 1000000.0;  // Convert to seconds
        
        return metrics;
    }
    
    /**
     * Print evaluation results
     * @param metrics Evaluation metrics
     * @param image_name Name of the image being evaluated
     */
    void printResults(const EvaluationMetrics& metrics, const std::string& image_name = "") const {
        std::cout << std::fixed << std::setprecision(4);
        if (!image_name.empty()) {
            std::cout << "Image: " << image_name << std::endl;
        }
        std::cout << "  Precision: " << metrics.precision << std::endl;
        std::cout << "  Recall: " << metrics.recall << std::endl;
        std::cout << "  F-Measure: " << metrics.f_measure << std::endl;
        std::cout << "  GT: " << metrics.total_gt << ", Detected: " << metrics.total_detected << std::endl;
        std::cout << "  TP: " << metrics.true_positives << ", FP: " << metrics.false_positives 
                  << ", FN: " << metrics.false_negatives << std::endl;
        std::cout << "  Avg Overlap: " << metrics.average_overlap << std::endl;
        std::cout << "  Processing Time: " << metrics.processing_time << "s" << std::endl;
        std::cout << std::endl;
    }
};

/**
 * Main function for testing the evaluator
 */
int main() {
    std::cout << "CMED Ellipse Detection Evaluator (C++)" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Example usage
    cv::Size image_size(800, 600);
    CMEDEvaluator evaluator(0.8, "A1");
    
    // Test with sample data (you would replace these with actual file paths)
    std::string gt_file = "sample_gt.txt";
    std::string det_file = "sample_det.txt";
    
    if (fs::exists(gt_file) && fs::exists(det_file)) {
        EvaluationMetrics metrics = evaluator.evaluateSingleImage(gt_file, det_file, image_size);
        evaluator.printResults(metrics, "Sample Image");
    } else {
        std::cout << "Sample files not found. Please provide valid ground truth and detection files." << std::endl;
    }
    
    return 0;
}
