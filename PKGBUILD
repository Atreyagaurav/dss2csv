# Maintainer: Gaurav Atreya <allmanpride@gmail.com>
pkgname=dss2csv
pkgver=0.7
pkgrel=2
pkgdesc="Conversion from HEC-DSS file to ASCII and vice versa"
arch=('x86_64')
license=('GPL3')
depends=()
makedepends=('gcc-libs')
OPTIONS=(strip !debug)

build() {
    cd "$srcdir"
    make
}

package() {
    cd "$srcdir"
    mkdir -p "$pkgdir/usr/bin"
    mkdir -p "$pkgdir/usr/lib"
    mkdir -p "$pkgdir/usr/share/bash-completion/completions"
    cp "dss2csv" "$pkgdir/usr/bin/dss2csv"
    cp "csv2dss" "$pkgdir/usr/bin/csv2dss"
    cp "libhecdss.so" "$pkgdir/usr/lib/libhecdss.so"
    cp "bash-completions.sh" "$pkgdir/usr/share/bash-completion/completions/dss2csv"
}
