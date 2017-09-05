import sys, argparse
from html.parser import HTMLParser

class Parser(HTMLParser):
    def handle_starttag(self, tag, attrs):
        print('start: {}'.format(tag))

    def handle_endtag(self, tag):
        print('end: {}'.format(tag))

    def handle_data(self, data):
        print('data: {!r}'.format(data[:10]))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input')
    args = ap.parse_args()

    parser = Parser()
    with open(args.input, 'r') as fin:
        parser.feed(fin.read())
    return 0

if __name__ == '__main__':
    sys.exit(main())

