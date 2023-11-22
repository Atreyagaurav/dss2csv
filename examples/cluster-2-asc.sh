for i in $(seq 1 24);
do
    echo gdal_translate -of aaigrid -a_nodata 0 netcdf:cluster-1_weighted-prec-hec.nc:time${i} cluster1/SHG_POTOMACRIVER_PRECIP_31JUL2011:$(printf '%02d' $((i - 1)))00_31JUL2011:$(printf '%02d' $i)00_RADAR.asc
done
