"""
CMED Ellipse Detection Evaluator
================================

A comprehensive evaluation framework for the Characteristic Mapping for 
Ellipse Detection (CMED) algorithm.

This module provides tools to evaluate ellipse detection performance using
various metrics including F-measure, precision, recall, and overlap ratios.

Author: Chen Xingqiang
Date: 2024
"""

import numpy as np
import os
import glob
import logging
from typing import Dict, List, Tuple, Optional, Union
from dataclasses import dataclass
from pathlib import Path
import json
import time

from ellipse_overlap_calculator import EllipseOverlapCalculator

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


@dataclass
class EvaluationMetrics:
    """Container for evaluation metrics."""
    precision: float
    recall: float
    f_measure: float
    total_gt: int
    total_detected: int
    true_positives: int
    false_positives: int
    false_negatives: int
    average_overlap: float
    processing_time: float


class CMEDEvaluator:
    """
    Main evaluator class for CMED ellipse detection algorithm.
    
    Supports multiple evaluation methods:
    - A1: Lu-Arc evaluation method (TP accumulation)
    - B2: Jia-TIP evaluation method (per-image F-measure averaging)
    """
    
    def __init__(self, beta_threshold: float = 0.8, evaluation_method: str = 'A1'):
        """
        Initialize the evaluator.
        
        Args:
            beta_threshold: Overlap threshold for considering a detection as correct
            evaluation_method: 'A1' for Lu-Arc method, 'B2' for Jia-TIP method
        """
        self.beta_threshold = beta_threshold
        self.evaluation_method = evaluation_method
        self.results = {}
        
        logger.info(f"Initialized CMED Evaluator with beta={beta_threshold}, method={evaluation_method}")
    
    def load_ellipse_data(self, file_path: str) -> np.ndarray:
        """
        Load ellipse data from text file.
        
        Expected format:
        - First line: number of ellipses
        - Subsequent lines: ellipse parameters [a, b, x, y, theta]
        
        Args:
            file_path: Path to the ellipse data file
            
        Returns:
            Array of shape (5, N) containing N ellipses
        """
        try:
            data = np.loadtxt(file_path)
            if len(data.shape) == 1:
                # Single ellipse
                return data.reshape(5, 1)
            else:
                # Multiple ellipses
                num_ellipses = int(data[0])
                ellipse_params = data[1:].reshape(5, num_ellipses)
                return ellipse_params
        except Exception as e:
            logger.error(f"Error loading ellipse data from {file_path}: {e}")
            return np.empty((5, 0))
    
    def sort_ellipse_parameters(self, ellipses: np.ndarray) -> np.ndarray:
        """
        Sort ellipse parameters for consistent comparison.
        
        Args:
            ellipses: Array of shape (5, N) containing ellipse parameters
            
        Returns:
            Sorted ellipse parameters in format [a, b, x, y, theta]
        """
        if ellipses.size == 0:
            return ellipses
        
        # Transpose to sort by center coordinates
        ellipses_t = ellipses.T
        sorted_indices = np.lexsort((ellipses_t[:, 3], ellipses_t[:, 2]))  # Sort by y, then x
        sorted_ellipses = ellipses_t[sorted_indices].T
        
        # Ensure format is [a, b, x, y, theta]
        return sorted_ellipses
    
    def calculate_overlap_matrix(self, ground_truth: np.ndarray, 
                                detected: np.ndarray, 
                                image_size: Tuple[int, int]) -> np.ndarray:
        """
        Calculate overlap matrix between ground truth and detected ellipses.
        
        Args:
            ground_truth: Ground truth ellipses (5, N)
            detected: Detected ellipses (5, M)
            image_size: Image dimensions (width, height)
            
        Returns:
            Overlap matrix of shape (N, M)
        """
        if ground_truth.size == 0 or detected.size == 0:
            return np.empty((ground_truth.shape[1] if ground_truth.size > 0 else 0,
                           detected.shape[1] if detected.size > 0 else 0))
        
        calculator = EllipseOverlapCalculator(image_size)
        return calculator.calculate_overlap_matrix(ground_truth, detected)
    
    def evaluate_single_image(self, ground_truth_file: str, 
                             detected_file: str, 
                             image_size: Tuple[int, int]) -> EvaluationMetrics:
        """
        Evaluate ellipse detection for a single image.
        
        Args:
            ground_truth_file: Path to ground truth file
            detected_file: Path to detected ellipses file
            image_size: Image dimensions (width, height)
            
        Returns:
            Evaluation metrics for the image
        """
        start_time = time.time()
        
        # Load data
        ground_truth = self.load_ellipse_data(ground_truth_file)
        detected = self.load_ellipse_data(detected_file)
        
        # Sort parameters for consistent comparison
        ground_truth = self.sort_ellipse_parameters(ground_truth)
        detected = self.sort_ellipse_parameters(detected)
        
        # Handle empty cases
        if ground_truth.size == 0 and detected.size == 0:
            return EvaluationMetrics(
                precision=1.0, recall=1.0, f_measure=1.0,
                total_gt=0, total_detected=0,
                true_positives=0, false_positives=0, false_negatives=0,
                average_overlap=1.0, processing_time=time.time() - start_time
            )
        
        if ground_truth.size == 0:
            return EvaluationMetrics(
                precision=0.0, recall=1.0, f_measure=0.0,
                total_gt=0, total_detected=detected.shape[1],
                true_positives=0, false_positives=detected.shape[1], false_negatives=0,
                average_overlap=0.0, processing_time=time.time() - start_time
            )
        
        if detected.size == 0:
            return EvaluationMetrics(
                precision=1.0, recall=0.0, f_measure=0.0,
                total_gt=ground_truth.shape[1], total_detected=0,
                true_positives=0, false_positives=0, false_negatives=ground_truth.shape[1],
                average_overlap=0.0, processing_time=time.time() - start_time
            )
        
        # Calculate overlap matrix
        overlap_matrix = self.calculate_overlap_matrix(ground_truth, detected, image_size)
        
        # Calculate metrics based on evaluation method
        if self.evaluation_method == 'A1':
            # Lu-Arc method: TP accumulation
            true_positives = np.sum(np.max(overlap_matrix, axis=1) > self.beta_threshold)
            false_negatives = ground_truth.shape[1] - true_positives
            false_positives = detected.shape[1] - np.sum(np.max(overlap_matrix, axis=0) > self.beta_threshold)
        else:  # B2 method
            # Jia-TIP method: per-image F-measure
            true_positives = np.sum(np.max(overlap_matrix, axis=1) > self.beta_threshold)
            false_negatives = ground_truth.shape[1] - true_positives
            false_positives = detected.shape[1] - true_positives
        
        # Calculate precision, recall, and F-measure
        precision = true_positives / (true_positives + false_positives) if (true_positives + false_positives) > 0 else 0.0
        recall = true_positives / (true_positives + false_negatives) if (true_positives + false_negatives) > 0 else 0.0
        f_measure = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0.0
        
        # Calculate average overlap
        average_overlap = np.mean(overlap_matrix) if overlap_matrix.size > 0 else 0.0
        
        processing_time = time.time() - start_time
        
        return EvaluationMetrics(
            precision=precision, recall=recall, f_measure=f_measure,
            total_gt=ground_truth.shape[1], total_detected=detected.shape[1],
            true_positives=true_positives, false_positives=false_positives, false_negatives=false_negatives,
            average_overlap=average_overlap, processing_time=processing_time
        )
    
    def evaluate_dataset(self, dataset_path: str, 
                        ground_truth_dir: str = 'gt0',
                        detected_dir: str = 'det') -> Dict[str, Union[float, List[EvaluationMetrics]]]:
        """
        Evaluate ellipse detection on an entire dataset.
        
        Args:
            dataset_path: Path to dataset directory
            ground_truth_dir: Subdirectory containing ground truth files
            detected_dir: Subdirectory containing detection results
            
        Returns:
            Dictionary containing overall metrics and per-image results
        """
        logger.info(f"Starting evaluation of dataset: {dataset_path}")
        
        gt_path = os.path.join(dataset_path, ground_truth_dir)
        det_path = os.path.join(dataset_path, detected_dir)
        
        if not os.path.exists(gt_path):
            raise FileNotFoundError(f"Ground truth directory not found: {gt_path}")
        if not os.path.exists(det_path):
            raise FileNotFoundError(f"Detection directory not found: {det_path}")
        
        # Get all ground truth files
        gt_files = sorted(glob.glob(os.path.join(gt_path, '*.txt')))
        det_files = sorted(glob.glob(os.path.join(det_path, '*.txt')))
        
        if len(gt_files) != len(det_files):
            logger.warning(f"Number of GT files ({len(gt_files)}) != number of det files ({len(det_files)})")
        
        # Evaluate each image
        image_metrics = []
        total_processing_time = 0.0
        
        for i, gt_file in enumerate(gt_files):
            if i < len(det_files):
                det_file = det_files[i]
                
                # Estimate image size (you might want to load actual image)
                # For now, using a default size
                image_size = (800, 600)  # Default size, should be adjusted
                
                try:
                    metrics = self.evaluate_single_image(gt_file, det_file, image_size)
                    image_metrics.append(metrics)
                    total_processing_time += metrics.processing_time
                    
                    logger.debug(f"Image {i+1}: F={metrics.f_measure:.4f}, P={metrics.precision:.4f}, R={metrics.recall:.4f}")
                    
                except Exception as e:
                    logger.error(f"Error evaluating image {i+1}: {e}")
                    continue
        
        # Calculate overall metrics
        if self.evaluation_method == 'A1':
            # Lu-Arc method: accumulate TP, FP, FN across all images
            total_tp = sum(m.true_positives for m in image_metrics)
            total_fp = sum(m.false_positives for m in image_metrics)
            total_fn = sum(m.false_negatives for m in image_metrics)
            
            overall_precision = total_tp / (total_tp + total_fp) if (total_tp + total_fp) > 0 else 0.0
            overall_recall = total_tp / (total_tp + total_fn) if (total_tp + total_fn) > 0 else 0.0
            overall_f_measure = 2 * overall_precision * overall_recall / (overall_precision + overall_recall) if (overall_precision + overall_recall) > 0 else 0.0
            
        else:  # B2 method
            # Jia-TIP method: average per-image F-measures
            overall_precision = np.mean([m.precision for m in image_metrics]) if image_metrics else 0.0
            overall_recall = np.mean([m.recall for m in image_metrics]) if image_metrics else 0.0
            overall_f_measure = np.mean([m.f_measure for m in image_metrics]) if image_metrics else 0.0
        
        overall_metrics = {
            'precision': overall_precision,
            'recall': overall_recall,
            'f_measure': overall_f_measure,
            'total_images': len(image_metrics),
            'total_processing_time': total_processing_time,
            'average_processing_time': total_processing_time / len(image_metrics) if image_metrics else 0.0,
            'evaluation_method': self.evaluation_method,
            'beta_threshold': self.beta_threshold
        }
        
        logger.info(f"Dataset evaluation completed:")
        logger.info(f"  Precision: {overall_precision:.4f}")
        logger.info(f"  Recall: {overall_recall:.4f}")
        logger.info(f"  F-Measure: {overall_f_measure:.4f}")
        logger.info(f"  Total images: {len(image_metrics)}")
        logger.info(f"  Processing time: {total_processing_time:.2f}s")
        
        return {
            'overall_metrics': overall_metrics,
            'image_metrics': image_metrics
        }
    
    def save_results(self, results: Dict, output_path: str):
        """
        Save evaluation results to JSON file.
        
        Args:
            results: Evaluation results dictionary
            output_path: Path to save the results
        """
        # Convert dataclass objects to dictionaries for JSON serialization
        serializable_results = {
            'overall_metrics': results['overall_metrics'],
            'image_metrics': [
                {
                    'precision': m.precision,
                    'recall': m.recall,
                    'f_measure': m.f_measure,
                    'total_gt': m.total_gt,
                    'total_detected': m.total_detected,
                    'true_positives': m.true_positives,
                    'false_positives': m.false_positives,
                    'false_negatives': m.false_negatives,
                    'average_overlap': m.average_overlap,
                    'processing_time': m.processing_time
                }
                for m in results['image_metrics']
            ]
        }
        
        with open(output_path, 'w') as f:
            json.dump(serializable_results, f, indent=2)
        
        logger.info(f"Results saved to: {output_path}")


def main():
    """Example usage of the CMED evaluator."""
    # Example dataset paths
    dataset_paths = [
        './datasets/ged',
        './datasets/Prasad_plus',
        './datasets/Traffic Sign Dataset'
    ]
    
    # Evaluate with different methods
    for method in ['A1', 'B2']:
        logger.info(f"\n{'='*50}")
        logger.info(f"Evaluating with method: {method}")
        logger.info(f"{'='*50}")
        
        evaluator = CMEDEvaluator(beta_threshold=0.8, evaluation_method=method)
        
        for dataset_path in dataset_paths:
            if os.path.exists(dataset_path):
                try:
                    results = evaluator.evaluate_dataset(dataset_path)
                    
                    # Save results
                    output_file = f"evaluation_results_{method}_{os.path.basename(dataset_path)}.json"
                    evaluator.save_results(results, output_file)
                    
                except Exception as e:
                    logger.error(f"Error evaluating dataset {dataset_path}: {e}")
            else:
                logger.warning(f"Dataset not found: {dataset_path}")


if __name__ == "__main__":
    main()
