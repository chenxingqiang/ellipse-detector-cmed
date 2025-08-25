"""
Ellipse Overlap Calculator for CMED Evaluation
==============================================

This module provides functions to calculate overlap ratios between ellipses
for evaluating ellipse detection algorithms.

Author: Chen Xingqiang
Date: 2024
"""

import numpy as np
import cv2
from typing import Tuple, List, Optional
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class EllipseOverlapCalculator:
    """
    A class to calculate overlap ratios between ellipses for evaluation purposes.
    
    This class implements the overlap calculation algorithm used in the CMED
    (Characteristic Mapping for Ellipse Detection) evaluation framework.
    """
    
    def __init__(self, image_size: Tuple[int, int]):
        """
        Initialize the calculator with image dimensions.
        
        Args:
            image_size: Tuple of (width, height) of the image
        """
        self.image_width, self.image_height = image_size
        self._create_coordinate_grid()
    
    def _create_coordinate_grid(self):
        """Create coordinate grids for pixel-wise calculations."""
        x_coords, y_coords = np.meshgrid(
            np.arange(1, self.image_width + 1),
            np.arange(1, self.image_height + 1)
        )
        self.pixels_x = x_coords
        self.pixels_y = y_coords
    
    def calculate_ellipse_mask(self, ellipse_params: np.ndarray) -> np.ndarray:
        """
        Calculate binary mask for an ellipse.
        
        Args:
            ellipse_params: Array [a, b, x, y, theta] where:
                a: semi-major axis
                b: semi-minor axis  
                x, y: center coordinates
                theta: rotation angle in radians
        
        Returns:
            Binary mask where True indicates pixels inside the ellipse
        """
        a, b, x, y, theta = ellipse_params
        
        # Calculate ellipse equation: ((x-x0)*sin(θ)-(y-y0)*cos(θ))²/b² + ((x-x0)*cos(θ)+(y-y0)*sin(θ))²/a² ≤ 1
        sin_theta = np.sin(theta)
        cos_theta = np.cos(theta)
        
        # Transform coordinates
        dx = self.pixels_x - x
        dy = self.pixels_y - y
        
        # Apply rotation and scaling
        term1 = (dx * sin_theta - dy * cos_theta) ** 2 / (b ** 2)
        term2 = (dx * cos_theta + dy * sin_theta) ** 2 / (a ** 2)
        
        # Ellipse equation: term1 + term2 ≤ 1
        ellipse_equation = term1 + term2 - 1
        
        # Return mask where pixels are inside ellipse (equation ≤ 0)
        return ellipse_equation <= 0
    
    def calculate_overlap_ratio(self, ellipse1_params: np.ndarray, 
                               ellipse2_params: np.ndarray) -> float:
        """
        Calculate overlap ratio between two ellipses.
        
        Args:
            ellipse1_params: Parameters of first ellipse [a, b, x, y, theta]
            ellipse2_params: Parameters of second ellipse [a, b, x, y, theta]
        
        Returns:
            Overlap ratio between 0 and 1, where 1 means perfect overlap
        """
        try:
            # Calculate masks for both ellipses
            mask1 = self.calculate_ellipse_mask(ellipse1_params)
            mask2 = self.calculate_ellipse_mask(ellipse2_params)
            
            # Calculate overlap using set operations
            # XOR gives pixels that are in exactly one ellipse
            xor_pixels = np.logical_xor(mask1, mask2)
            # OR gives pixels that are in either ellipse
            or_pixels = np.logical_or(mask1, mask2)
            
            # Calculate overlap ratio: 1 - (XOR pixels / OR pixels)
            xor_count = np.sum(xor_pixels)
            or_count = np.sum(or_pixels)
            
            if or_count == 0:
                return 0.0
            
            overlap_ratio = 1.0 - (xor_count / or_count)
            return float(overlap_ratio)
            
        except Exception as e:
            logger.error(f"Error calculating overlap ratio: {e}")
            return 0.0
    
    def calculate_overlap_matrix(self, ground_truth_ellipses: np.ndarray,
                                detected_ellipses: np.ndarray) -> np.ndarray:
        """
        Calculate overlap matrix between ground truth and detected ellipses.
        
        Args:
            ground_truth_ellipses: Array of shape (5, N) for N ground truth ellipses
            detected_ellipses: Array of shape (5, M) for M detected ellipses
        
        Returns:
            Overlap matrix of shape (N, M)
        """
        num_gt = ground_truth_ellipses.shape[1]
        num_det = detected_ellipses.shape[1]
        
        overlap_matrix = np.zeros((num_gt, num_det))
        
        for i in range(num_gt):
            for j in range(num_det):
                overlap_matrix[i, j] = self.calculate_overlap_ratio(
                    ground_truth_ellipses[:, i],
                    detected_ellipses[:, j]
                )
        
        return overlap_matrix


def calculate_overlap_ratio_simple(ellipse1_params: np.ndarray,
                                  ellipse2_params: np.ndarray,
                                  image_size: Tuple[int, int]) -> float:
    """
    Simple function to calculate overlap ratio between two ellipses.
    
    This is a simplified interface for the EllipseOverlapCalculator class.
    
    Args:
        ellipse1_params: Parameters of first ellipse [a, b, x, y, theta]
        ellipse2_params: Parameters of second ellipse [a, b, x, y, theta]
        image_size: Tuple of (width, height) of the image
    
    Returns:
        Overlap ratio between 0 and 1
    """
    calculator = EllipseOverlapCalculator(image_size)
    return calculator.calculate_overlap_ratio(ellipse1_params, ellipse2_params)


# Example usage and testing
if __name__ == "__main__":
    # Test the overlap calculator
    image_size = (100, 100)
    calculator = EllipseOverlapCalculator(image_size)
    
    # Test ellipses
    ellipse1 = np.array([20, 10, 50, 50, 0])  # Horizontal ellipse at center
    ellipse2 = np.array([15, 8, 55, 45, np.pi/4])  # Rotated ellipse nearby
    
    overlap = calculator.calculate_overlap_ratio(ellipse1, ellipse2)
    print(f"Overlap ratio: {overlap:.4f}")
    
    # Test with multiple ellipses
    gt_ellipses = np.array([[20, 10, 30, 30, 0],
                           [15, 8, 70, 70, np.pi/4]]).T
    
    det_ellipses = np.array([[19, 9, 31, 31, 0.1],
                            [16, 7, 69, 69, np.pi/4 + 0.1]]).T
    
    overlap_matrix = calculator.calculate_overlap_matrix(gt_ellipses, det_ellipses)
    print(f"Overlap matrix:\n{overlap_matrix}")
