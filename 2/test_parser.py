import sys

_section_header = '######## Section '
_case_header = '----# Test {'
_case_output = '----# Output'
_case_end = '----# }'

class TestCase:
    def __init__(self, name, line):
        self.name = name
        self.line = line
        self.body = ''
        self.output = ''

class TestSection:
    def __init__(self, name, section_type):
        self.name = name
        self.type = section_type
        self.cases = []

def parse(filename):
    with open(filename) as f:
        test_lines = f.read().splitlines()
    sections = []
    current_section = None
    current_case = None
    case_section = None
    pos = 0
    for line_i, line in enumerate(test_lines):
        line_i += 1
        if current_case:
            if case_section == 'body' and line.startswith(_case_output):
                case_section = 'output'
                continue
            if line.startswith(_case_end):
                current_section.cases.append(current_case)
                current_case = None
                continue
            line += '\n'
            if case_section == 'body':
                current_case.body += line
            elif case_section == 'output':
                current_case.output += line
            else:
                sys.exit('Unreachable')
            continue
        if line.startswith(_section_header):
            if current_section:
                sections.append(current_section)
                current_section = None
            section_name = line[len(_section_header):].strip()
            if not section_name:
                sys.exit('Section name on line {} is empty'
                    .format(line_i))
            if section_name.startswith('bonus'):
                section_type = section_name
            else:
                section_type = 'base'
            current_section = TestSection(section_name, section_type)
            continue
        if not line:
            continue
        if line.startswith(_case_header):
            case_name = line[len(_case_header):].strip(' -')
            if not case_name:
                sys.exit('Case name on line {} is empty'.format(line_i))
            current_case = TestCase(case_name, line_i)
            case_section = 'body'
            continue
        sys.exit('Unknown test syntax on line {}'.format(line_i))
    if current_case:
        sys.exit('Unfinished test case in the end of file')
    if current_section:
        sections.append(current_section)
    if not sections:
        sys.exit('No tests found')
    return sections
