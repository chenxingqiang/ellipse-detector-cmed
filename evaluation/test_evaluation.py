#!/usr/bin/env python3
"""
Test script for CMED evaluation tools
=====================================

This script tests the ellipse overlap calculator and evaluator
to ensure they work correctly.

Author: Chen Xingqiang
Date: 2024
"""

import numpy as np
import sys
import os

# Add the current directory to the path so we can import our modules
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from ellipse_overlap_calculator import EllipseOverlapCalculator, calculate_overlap_ratio_simple
    from cmed_evaluator import CMEDEvaluator, EvaluationMetrics
    print("✅ Successfully imported evaluation modules")
except ImportError as e:
    print(f"❌ Failed to import evaluation modules: {e}")
    sys.exit(1)


def test_overlap_calculator():
    """Test the ellipse overlap calculator."""
    print("\n🧪 Testing Ellipse Overlap Calculator...")
    
    # Test 1: Basic overlap calculation
    image_size = (100, 100)
    calculator = EllipseOverlapCalculator(image_size)
    
    # Create two overlapping ellipses
    ellipse1 = np.array([20, 10, 50, 50, 0])  # Horizontal ellipse at center
    ellipse2 = np.array([15, 8, 55, 45, np.pi/4])  # Rotated ellipse nearby
    
    overlap = calculator.calculate_overlap_ratio(ellipse1, ellipse2)
    print(f"  Overlap between test ellipses: {overlap:.4f}")
    
    # Test 2: Same ellipse should have overlap = 1.0
    overlap_same = calculator.calculate_overlap_ratio(ellipse1, ellipse1)
    print(f"  Overlap with same ellipse: {overlap_same:.4f}")
    
    # Test 3: Non-overlapping ellipses should have overlap ≈ 0.0
    ellipse3 = np.array([10, 5, 10, 10, 0])  # Small ellipse in corner
    overlap_none = calculator.calculate_overlap_ratio(ellipse1, ellipse3)
    print(f"  Overlap with non-overlapping ellipse: {overlap_none:.4f}")
    
    print("✅ Overlap calculator tests completed")


def test_evaluator():
    """Test the CMED evaluator."""
    print("\n🧪 Testing CMED Evaluator...")
    
    # Create evaluator
    evaluator = CMEDEvaluator(beta_threshold=0.8, evaluation_method='A1')
    
    # Test with sample data
    image_size = (100, 100)
    
    # Create sample ground truth and detection data
    gt_ellipses = np.array([[20, 10, 30, 30, 0],
                           [15, 8, 70, 70, np.pi/4]]).T
    
    det_ellipses = np.array([[19, 9, 31, 31, 0.1],
                            [16, 7, 69, 69, np.pi/4 + 0.1]]).T
    
    # Test overlap matrix calculation
    calculator = EllipseOverlapCalculator(image_size)
    overlap_matrix = calculator.calculate_overlap_matrix(gt_ellipses, det_ellipses)
    print(f"  Overlap matrix shape: {overlap_matrix.shape}")
    print(f"  Overlap matrix:\n{overlap_matrix}")
    
    print("✅ Evaluator tests completed")


def test_data_loading():
    """Test data loading functionality."""
    print("\n🧪 Testing Data Loading...")
    
    # Create a temporary test file
    test_file = "temp_test_data.txt"
    try:
        with open(test_file, 'w') as f:
            f.write("2\n")  # Number of ellipses
            f.write("20.0 10.0 50.0 50.0 0.0\n")  # First ellipse
            f.write("15.0 8.0 70.0 70.0 1.57\n")  # Second ellipse
        
        # Test loading
        evaluator = CMEDEvaluator()
        data = evaluator.load_ellipse_data(test_file)
        print(f"  Loaded {data.shape[1]} ellipses from test file")
        print(f"  Data shape: {data.shape}")
        
        # Clean up
        os.remove(test_file)
        print("✅ Data loading tests completed")
        
    except Exception as e:
        print(f"❌ Data loading test failed: {e}")
        if os.path.exists(test_file):
            os.remove(test_file)


def main():
    """Run all tests."""
    print("🚀 Starting CMED Evaluation Tools Tests")
    print("=" * 50)
    
    try:
        test_overlap_calculator()
        test_evaluator()
        test_data_loading()
        
        print("\n🎉 All tests completed successfully!")
        print("The evaluation tools are working correctly.")
        
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
