from setuptools import setup, Extension

def main():
    setup(name="tapes",
          version="1.0.0",
          description="Python interface for managing magnetic tapes and libraries",
          author="Piotr Piatyszek",
          author_email="piotr.piatyszek@pw.edu.pl",
          packages=["tapes"],
          ext_modules=[Extension("tapes.internal.changer", ["src/changer.c"]), Extension("tapes.internal.tape", ["src/tape.c"])])

if __name__ == "__main__":
    main()
