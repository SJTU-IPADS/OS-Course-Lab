#!/usr/bin/env python3
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

import pathlib
import jinja2
import argparse
import sys
from typing import Dict, List


class Project:
    name: str
    revision: str

    def __init__(self, name: str, revision: str):
        self.name = name
        self.revision = revision

    @staticmethod
    def from_arg(arg: str) -> 'Project':
        name, rev = arg.split(':')
        return Project(name, rev)


projects_list = ['kernel', 'libs', 'apps', 'system_services']
projects_repo = {
    'kernel': 'starkernel.git',
    'libs': 'staros-libs.git',
    'apps': 'staros-apps.git',
    'system_services': 'staros-system-services.git'
}
default_branch_name = "master"
main_repo = "main"
mr_labels_mapper = {
    'MR-link-apps': 'apps',
    'MR-link-libs': 'libs',
    'MR-link-system-services': 'system_services',
    'MR-link-kernel': 'kernel'
}
projects_branch = {
    proj:default_branch_name
    for proj in projects_repo
}


def parse_project_specs(args: argparse.Namespace, projects: List[str]) -> Dict[str, Project]:
    if args.self_component_name:
        self_branch = args.self_component_branch
        if args.self_component_name != main_repo:
            projects_branch[args.self_component_name] = self_branch
        mr_labels = [l for l in args.mr_labels.split(',') if l]
        for label in mr_labels:
            if label in mr_labels_mapper:
                projects_branch[mr_labels_mapper[label]] = self_branch
    print(projects_branch)
    return {
        proj: Project.from_arg(f"{projects_repo[proj]}:{projects_branch[proj]}")
        for proj in projects
    }


def generate_manifest_file(path: pathlib.Path, project_specs: Dict[str, Project]):
    jinja_env = jinja2.Environment(loader=jinja2.FileSystemLoader(sys.path[0]),
                                   autoescape=jinja2.select_autoescape
                                   )
    manifest_tmpl = jinja_env.get_template('manifest.xml')
    with path.open('w') as tmpl_file:
        manifest = manifest_tmpl.render(project_specs)
        tmpl_file.write(manifest)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', type=str,
                        help=argparse.SUPPRESS, default='default.xml')
    parser.add_argument('--dest', type=str,
                        help=argparse.SUPPRESS, default='./.repo/manifests')
    parser.add_argument('--self-component-name', type=str, metavar='repo_name', nargs='?',
                        help='name of component issuing the pipeline', default='', const='')
    parser.add_argument('--self-component-branch', type=str, metavar='branch_name', nargs='?',
                        help='branch of component issuing the pipeline', default='', const='')
    parser.add_argument('--mr-labels', type=str, metavar='label1,labe2,label3', nargs='?',
                        help='repo name and revision of system services repository', default='', const='')
    args = parser.parse_args()
    project_specs = parse_project_specs(args, projects_list)
    name, dest = pathlib.Path(args.name), pathlib.Path(args.dest)
    generate_manifest_file(dest / name, project_specs)


if __name__ == "__main__":
    main()
