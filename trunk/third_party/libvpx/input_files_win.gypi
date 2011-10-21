{
  'conditions': [
    ['target_arch=="ia32"', {
      'sources': [
        # generated input files
        'source/config/win/ia32/asm_com_offsets.asm',
        'source/config/win/ia32/asm_enc_offsets.asm',
        'source/config/win/ia32/asm_dec_offsets.asm',
        'source/config/win/ia32/vpx_config.asm',
        'source/config/win/ia32/vpx_config.c',
        'source/config/win/ia32/vpx_config.h',
        'source/config/win/ia32/vpx_version.h',

        # source input files
        'source/libvpx/vpx/src/vpx_decoder.c',
        'source/libvpx/vpx/src/vpx_decoder_compat.c',
        'source/libvpx/vpx/src/vpx_encoder.c',
        'source/libvpx/vpx/src/vpx_codec.c',
        'source/libvpx/vpx/src/vpx_image.c',
        'source/libvpx/vpx_mem/vpx_mem.c',
        'source/libvpx/vpx_scale/generic/vpxscale.c',
        'source/libvpx/vpx_scale/generic/yv12config.c',
        'source/libvpx/vpx_scale/generic/yv12extend.c',
        'source/libvpx/vpx_scale/generic/scalesystemdependent.c',
        'source/libvpx/vpx_scale/generic/gen_scalers.c',
        'source/libvpx/vp8/common/alloccommon.c',
        'source/libvpx/vp8/common/asm_com_offsets.c',
        'source/libvpx/vp8/common/blockd.c',
        'source/libvpx/vp8/common/debugmodes.c',
        'source/libvpx/vp8/common/entropy.c',
        'source/libvpx/vp8/common/entropymode.c',
        'source/libvpx/vp8/common/entropymv.c',
        'source/libvpx/vp8/common/extend.c',
        'source/libvpx/vp8/common/filter.c',
        'source/libvpx/vp8/common/findnearmv.c',
        'source/libvpx/vp8/common/generic/systemdependent.c',
        'source/libvpx/vp8/common/idctllm.c',
        'source/libvpx/vp8/common/invtrans.c',
        'source/libvpx/vp8/common/loopfilter.c',
        'source/libvpx/vp8/common/loopfilter_filters.c',
        'source/libvpx/vp8/common/mbpitch.c',
        'source/libvpx/vp8/common/modecont.c',
        'source/libvpx/vp8/common/modecontext.c',
        'source/libvpx/vp8/common/quant_common.c',
        'source/libvpx/vp8/common/recon.c',
        'source/libvpx/vp8/common/reconinter.c',
        'source/libvpx/vp8/common/reconintra.c',
        'source/libvpx/vp8/common/reconintra4x4.c',
        'source/libvpx/vp8/common/setupintrarecon.c',
        'source/libvpx/vp8/common/swapyv12buffer.c',
        'source/libvpx/vp8/common/treecoder.c',
        'source/libvpx/vp8/common/x86/x86_systemdependent.c',
        'source/libvpx/vp8/common/x86/vp8_asm_stubs.c',
        'source/libvpx/vp8/common/x86/loopfilter_x86.c',
        'source/libvpx/vp8/common/postproc.c',
        'source/libvpx/vp8/common/x86/recon_wrapper_sse2.c',
        'source/libvpx/vp8/vp8_cx_iface.c',
        'source/libvpx/vp8/encoder/asm_enc_offsets.c',
        'source/libvpx/vp8/encoder/bitstream.c',
        'source/libvpx/vp8/encoder/boolhuff.c',
        'source/libvpx/vp8/encoder/dct.c',
        'source/libvpx/vp8/encoder/encodeframe.c',
        'source/libvpx/vp8/encoder/encodeintra.c',
        'source/libvpx/vp8/encoder/encodemb.c',
        'source/libvpx/vp8/encoder/encodemv.c',
        'source/libvpx/vp8/encoder/ethreading.c',
        'source/libvpx/vp8/encoder/firstpass.c',
        'source/libvpx/vp8/encoder/generic/csystemdependent.c',
        'source/libvpx/vp8/encoder/lookahead.c',
        'source/libvpx/vp8/encoder/mcomp.c',
        'source/libvpx/vp8/encoder/modecosts.c',
        'source/libvpx/vp8/encoder/onyx_if.c',
        'source/libvpx/vp8/encoder/pickinter.c',
        'source/libvpx/vp8/encoder/picklpf.c',
        'source/libvpx/vp8/encoder/psnr.c',
        'source/libvpx/vp8/encoder/quantize.c',
        'source/libvpx/vp8/encoder/ratectrl.c',
        'source/libvpx/vp8/encoder/rdopt.c',
        'source/libvpx/vp8/encoder/sad_c.c',
        'source/libvpx/vp8/encoder/segmentation.c',
        'source/libvpx/vp8/encoder/tokenize.c',
        'source/libvpx/vp8/encoder/treewriter.c',
        'source/libvpx/vp8/encoder/variance_c.c',
        'source/libvpx/vp8/encoder/temporal_filter.c',
        'source/libvpx/vp8/encoder/x86/x86_csystemdependent.c',
        'source/libvpx/vp8/encoder/x86/variance_mmx.c',
        'source/libvpx/vp8/encoder/x86/variance_sse2.c',
        'source/libvpx/vp8/encoder/x86/variance_ssse3.c',
        'source/libvpx/vp8/vp8_dx_iface.c',
        'source/libvpx/vp8/decoder/asm_dec_offsets.c',
        'source/libvpx/vp8/decoder/dboolhuff.c',
        'source/libvpx/vp8/decoder/decodemv.c',
        'source/libvpx/vp8/decoder/decodframe.c',
        'source/libvpx/vp8/decoder/dequantize.c',
        'source/libvpx/vp8/decoder/detokenize.c',
        'source/libvpx/vp8/decoder/error_concealment.c',
        'source/libvpx/vp8/decoder/generic/dsystemdependent.c',
        'source/libvpx/vp8/decoder/onyxd_if.c',
        'source/libvpx/vp8/decoder/threading.c',
        'source/libvpx/vp8/decoder/idct_blk.c',
        'source/libvpx/vp8/decoder/reconintra_mt.c',
        'source/libvpx/vp8/decoder/x86/x86_dsystemdependent.c',
        'source/libvpx/vp8/decoder/x86/idct_blk_mmx.c',
        'source/libvpx/vp8/decoder/x86/idct_blk_sse2.c',
        'source/libvpx/vpx_ports/x86_cpuid.c',
        
        # yasm input
        'source/libvpx/vp8/common/x86/idctllm_mmx.asm',
        'source/libvpx/vp8/common/x86/iwalsh_mmx.asm',
        'source/libvpx/vp8/common/x86/recon_mmx.asm',
        'source/libvpx/vp8/common/x86/subpixel_mmx.asm',
        'source/libvpx/vp8/common/x86/loopfilter_mmx.asm',
        'source/libvpx/vp8/common/x86/idctllm_sse2.asm',
        'source/libvpx/vp8/common/x86/recon_sse2.asm',
        'source/libvpx/vp8/common/x86/subpixel_sse2.asm',
        'source/libvpx/vp8/common/x86/loopfilter_sse2.asm',
        'source/libvpx/vp8/common/x86/iwalsh_sse2.asm',
        'source/libvpx/vp8/common/x86/subpixel_ssse3.asm',
        'source/libvpx/vp8/common/x86/postproc_mmx.asm',
        'source/libvpx/vp8/common/x86/postproc_sse2.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_mmx.asm',
        'source/libvpx/vp8/encoder/x86/sad_mmx.asm',
        'source/libvpx/vp8/encoder/x86/dct_mmx.asm',
        'source/libvpx/vp8/encoder/x86/subtract_mmx.asm',
        'source/libvpx/vp8/encoder/x86/dct_sse2.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_sse2.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse2.asm',
        'source/libvpx/vp8/encoder/x86/fwalsh_sse2.asm',
        'source/libvpx/vp8/encoder/x86/quantize_sse2.asm',
        'source/libvpx/vp8/encoder/x86/subtract_sse2.asm',
        'source/libvpx/vp8/encoder/x86/temporal_filter_apply_sse2.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse3.asm',
        'source/libvpx/vp8/encoder/x86/sad_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/quantize_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse4.asm',
        'source/libvpx/vp8/encoder/x86/quantize_sse4.asm',
        'source/libvpx/vp8/encoder/x86/quantize_mmx.asm',
        'source/libvpx/vp8/encoder/x86/encodeopt.asm',
        'source/libvpx/vpx_ports/emms.asm',
        'source/libvpx/vpx_ports/x86_abi_support.asm',
        'source/libvpx/vp8/decoder/x86/dequantize_mmx.asm',
      ], # sources
    },], # target_arch=="ia32"
    ['target_arch=="x64"', {
      'sources': [
        # generated input files
        'source/config/win/x64/asm_com_offsets.asm',
        'source/config/win/x64/asm_dec_offsets.asm',
        'source/config/win/x64/asm_enc_offsets.asm',
        'source/config/win/x64/vpx_config.asm',
        'source/config/win/x64/vpx_config.c',
        'source/config/win/x64/vpx_config.h',
        'source/config/win/x64/vpx_version.h',

        # source input files
        'source/libvpx/vpx/src/vpx_decoder.c',
        'source/libvpx/vpx/src/vpx_decoder_compat.c',
        'source/libvpx/vpx/src/vpx_encoder.c',
        'source/libvpx/vpx/src/vpx_codec.c',
        'source/libvpx/vpx/src/vpx_image.c',
        'source/libvpx/vpx_mem/vpx_mem.c',
        'source/libvpx/vpx_scale/generic/vpxscale.c',
        'source/libvpx/vpx_scale/generic/yv12config.c',
        'source/libvpx/vpx_scale/generic/yv12extend.c',
        'source/libvpx/vpx_scale/generic/scalesystemdependent.c',
        'source/libvpx/vpx_scale/generic/gen_scalers.c',
        'source/libvpx/vp8/common/alloccommon.c',
        'source/libvpx/vp8/common/asm_com_offsets.c',
        'source/libvpx/vp8/common/blockd.c',
        'source/libvpx/vp8/common/debugmodes.c',
        'source/libvpx/vp8/common/entropy.c',
        'source/libvpx/vp8/common/entropymode.c',
        'source/libvpx/vp8/common/entropymv.c',
        'source/libvpx/vp8/common/extend.c',
        'source/libvpx/vp8/common/filter.c',
        'source/libvpx/vp8/common/findnearmv.c',
        'source/libvpx/vp8/common/generic/systemdependent.c',
        'source/libvpx/vp8/common/idctllm.c',
        'source/libvpx/vp8/common/invtrans.c',
        'source/libvpx/vp8/common/loopfilter.c',
        'source/libvpx/vp8/common/loopfilter_filters.c',
        'source/libvpx/vp8/common/mbpitch.c',
        'source/libvpx/vp8/common/modecont.c',
        'source/libvpx/vp8/common/modecontext.c',
        'source/libvpx/vp8/common/quant_common.c',
        'source/libvpx/vp8/common/recon.c',
        'source/libvpx/vp8/common/reconinter.c',
        'source/libvpx/vp8/common/reconintra.c',
        'source/libvpx/vp8/common/reconintra4x4.c',
        'source/libvpx/vp8/common/setupintrarecon.c',
        'source/libvpx/vp8/common/swapyv12buffer.c',
        'source/libvpx/vp8/common/treecoder.c',
        'source/libvpx/vp8/common/x86/x86_systemdependent.c',
        'source/libvpx/vp8/common/x86/vp8_asm_stubs.c',
        'source/libvpx/vp8/common/x86/loopfilter_x86.c',
        'source/libvpx/vp8/common/postproc.c',
        'source/libvpx/vp8/common/x86/recon_wrapper_sse2.c',
        'source/libvpx/vp8/vp8_cx_iface.c',
        'source/libvpx/vp8/encoder/asm_enc_offsets.c',
        'source/libvpx/vp8/encoder/bitstream.c',
        'source/libvpx/vp8/encoder/boolhuff.c',
        'source/libvpx/vp8/encoder/dct.c',
        'source/libvpx/vp8/encoder/encodeframe.c',
        'source/libvpx/vp8/encoder/encodeintra.c',
        'source/libvpx/vp8/encoder/encodemb.c',
        'source/libvpx/vp8/encoder/encodemv.c',
        'source/libvpx/vp8/encoder/ethreading.c',
        'source/libvpx/vp8/encoder/firstpass.c',
        'source/libvpx/vp8/encoder/generic/csystemdependent.c',
        'source/libvpx/vp8/encoder/lookahead.c',
        'source/libvpx/vp8/encoder/mcomp.c',
        'source/libvpx/vp8/encoder/modecosts.c',
        'source/libvpx/vp8/encoder/onyx_if.c',
        'source/libvpx/vp8/encoder/pickinter.c',
        'source/libvpx/vp8/encoder/picklpf.c',
        'source/libvpx/vp8/encoder/psnr.c',
        'source/libvpx/vp8/encoder/quantize.c',
        'source/libvpx/vp8/encoder/ratectrl.c',
        'source/libvpx/vp8/encoder/rdopt.c',
        'source/libvpx/vp8/encoder/sad_c.c',
        'source/libvpx/vp8/encoder/segmentation.c',
        'source/libvpx/vp8/encoder/tokenize.c',
        'source/libvpx/vp8/encoder/treewriter.c',
        'source/libvpx/vp8/encoder/variance_c.c',
        'source/libvpx/vp8/encoder/temporal_filter.c',
        'source/libvpx/vp8/encoder/x86/x86_csystemdependent.c',
        'source/libvpx/vp8/encoder/x86/variance_mmx.c',
        'source/libvpx/vp8/encoder/x86/variance_sse2.c',
        'source/libvpx/vp8/encoder/x86/variance_ssse3.c',
        'source/libvpx/vp8/vp8_dx_iface.c',
        'source/libvpx/vp8/decoder/asm_dec_offsets.c',
        'source/libvpx/vp8/decoder/dboolhuff.c',
        'source/libvpx/vp8/decoder/decodemv.c',
        'source/libvpx/vp8/decoder/decodframe.c',
        'source/libvpx/vp8/decoder/dequantize.c',
        'source/libvpx/vp8/decoder/detokenize.c',
        'source/libvpx/vp8/decoder/error_concealment.c',
        'source/libvpx/vp8/decoder/generic/dsystemdependent.c',
        'source/libvpx/vp8/decoder/onyxd_if.c',
        'source/libvpx/vp8/decoder/threading.c',
        'source/libvpx/vp8/decoder/idct_blk.c',
        'source/libvpx/vp8/decoder/reconintra_mt.c',
        'source/libvpx/vp8/decoder/x86/x86_dsystemdependent.c',
        'source/libvpx/vp8/decoder/x86/idct_blk_mmx.c',
        'source/libvpx/vp8/decoder/x86/idct_blk_sse2.c',
        'source/libvpx/vpx_ports/x86_cpuid.c',

        # yasm input
        'source/libvpx/vp8/common/x86/idctllm_mmx.asm',
        'source/libvpx/vp8/common/x86/iwalsh_mmx.asm',
        'source/libvpx/vp8/common/x86/recon_mmx.asm',
        'source/libvpx/vp8/common/x86/subpixel_mmx.asm',
        'source/libvpx/vp8/common/x86/loopfilter_mmx.asm',
        'source/libvpx/vp8/common/x86/idctllm_sse2.asm',
        'source/libvpx/vp8/common/x86/recon_sse2.asm',
        'source/libvpx/vp8/common/x86/subpixel_sse2.asm',
        'source/libvpx/vp8/common/x86/loopfilter_sse2.asm',
        'source/libvpx/vp8/common/x86/iwalsh_sse2.asm',
        'source/libvpx/vp8/common/x86/subpixel_ssse3.asm',
        'source/libvpx/vp8/common/x86/postproc_mmx.asm',
        'source/libvpx/vp8/common/x86/postproc_sse2.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_mmx.asm',
        'source/libvpx/vp8/encoder/x86/sad_mmx.asm',
        'source/libvpx/vp8/encoder/x86/dct_mmx.asm',
        'source/libvpx/vp8/encoder/x86/subtract_mmx.asm',
        'source/libvpx/vp8/encoder/x86/dct_sse2.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_sse2.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse2.asm',
        'source/libvpx/vp8/encoder/x86/fwalsh_sse2.asm',
        'source/libvpx/vp8/encoder/x86/quantize_sse2.asm',
        'source/libvpx/vp8/encoder/x86/subtract_sse2.asm',
        'source/libvpx/vp8/encoder/x86/temporal_filter_apply_sse2.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse3.asm',
        'source/libvpx/vp8/encoder/x86/sad_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/variance_impl_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/quantize_ssse3.asm',
        'source/libvpx/vp8/encoder/x86/sad_sse4.asm',
        'source/libvpx/vp8/encoder/x86/quantize_sse4.asm',
        'source/libvpx/vp8/encoder/x86/quantize_mmx.asm',
        'source/libvpx/vp8/encoder/x86/encodeopt.asm',
        'source/libvpx/vp8/encoder/x86/ssim_opt.asm',
        'source/libvpx/vpx_ports/emms.asm',
        'source/libvpx/vpx_ports/x86_abi_support.asm',
        'source/libvpx/vp8/decoder/x86/dequantize_mmx.asm',
      ], # sources
    },], # target_arch=="x64"
  ], # conditions
}
