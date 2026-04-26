#!/usr/bin/env python3
"""
Tool to extract and display HTML from captive-portal.h for preview.
Usage: python3 show_html.py [path_to_captive-portal.h]
"""

import sys
import re

def extract_html(h_file_path):
    """Extract HTML content from the C header file."""
    try:
        with open(h_file_path, 'r') as f:
            content = f.read()
        
        # Find content between R"rawliteral( and )rawliteral"
        pattern = r'R"rawliteral\((.*?)\)rawliteral"'
        match = re.search(pattern, content, re.DOTALL)
        
        if not match:
            print("Error: Could not find raw literal HTML content.")
            return None
        
        html_content = match.group(1)
        return html_content
    
    except FileNotFoundError:
        print(f"Error: File '{h_file_path}' not found.")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def main():
    # Default path
    h_file = "/home/priamai/sesame-robot/firmware/captive-portal.h"
    
    # Allow override from command line
    if len(sys.argv) > 1:
        h_file = sys.argv[1]
    
    print(f"Extracting HTML from: {h_file}\n")
    print("=" * 80)
    
    html = extract_html(h_file)
    
    if html:
        print(html)
        print("\n" + "=" * 80)
        print(f"\nHTML extracted successfully ({len(html)} characters)")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
