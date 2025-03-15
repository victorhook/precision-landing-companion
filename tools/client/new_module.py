#!/usr/bin/env python3

import sys
from pathlib import Path
import textwrap

ROOT_PATH = Path(__file__).absolute().parent
SRC = ROOT_PATH.joinpath('src')

DEFAULT_JS_FILE = r'''
import React from "react";
import './MODULE_NAME.css';

const MODULE_NAME = () => {
  return (
    <div className="MODULE_NAME">
        MODULE_NAME
    </div>
  );
};

export default MODULE_NAME;
'''

DEFAULT_CSS_FILE = r'''
.MODULE_NAME {
    border: 1px solid black;
}
'''

if __name__ == '__main__':
    if len(sys.argv) > 1:
        module_name = sys.argv[1]
    else:
        module_name = input('> Module name: ')

    print(f'Using module name "{module_name}"')

    module_dir = SRC.joinpath(module_name)
    if module_dir.exists():
        print('Module already exists, not creating!')
        sys.exit(0)

    module_dir.mkdir()
    with open(module_dir.joinpath(module_name + '.css'), 'w') as f:
        text = DEFAULT_CSS_FILE.replace('MODULE_NAME', module_name)
        f.write(text.lstrip())

    with open(module_dir.joinpath(module_name + '.js'), 'w') as f:
        text = DEFAULT_JS_FILE.replace('MODULE_NAME', module_name)
        f.write(text.lstrip())

    print(f'Done, created new module at {module_dir}')
        

