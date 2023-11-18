# Maintainer: Gaurav Atreya <allmanpride@gmail.com>
pkgname=dss2csv
pkgver=0.1
pkgrel=1
pkgdesc="Extract CSVs from a HEC-DSS file"
arch=('x86_64')
license=('GPL3')
depends=()
makedepends=('gcc-libs')

build() {
    cd "$srcdir"
    make
}

package() {
    cd "$srcdir"
    mkdir -p "$pkgdir/usr/bin"
    mkdir -p "$pkgdir/usr/lib"
    cp "${pkgname}" "$pkgdir/usr/bin/${pkgname}"
    cp "libhecdss.so" "$pkgdir/usr/lib/libhecdss.so"
}
