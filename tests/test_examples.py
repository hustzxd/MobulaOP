import mobula
import importlib
import runpy
import os
import sys


def packages_exist(pkg_names):
    for pkg in pkg_names:
        try:
            importlib.import_module(pkg)
        except ImportError:
            return False
    return True


def test_examples():
    EXAMPLES_PATH = os.path.join(os.path.dirname(__file__), '../examples')
    examples = [
        ([], ['MyFirstOP']),
        (['mxnet'], ['ConstantOP', 'RunROIAlign',
                     'dynamic_import_op/dynamic_import_op']),
    ]
    sys.path.append('./')
    for dep_pkgs, examples in examples:
        if packages_exist(dep_pkgs):
            for example in examples:
                print('testing... {}'.format(example))
                subpath, mod_name = os.path.split(example)
                fullpath = os.path.join(EXAMPLES_PATH, subpath)
                old_workpath = os.getcwd()
                os.chdir(fullpath)
                runpy.run_module(mod_name, {}, '__main__')
                os.chdir(old_workpath)