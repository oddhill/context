<?php if ($editable): ?>
  <div class='context-block-region <?php print $class ?>' id='context-block-region-<?php print $region ?>'>
    <div class='target'><?php print $region ?></div>
    <?php foreach ($blocks as $block): ?>
      <?php print theme('context_block_editable_block', $block); ?>
    <?php endforeach; ?>
  </div>
<?php else: ?>
  <?php foreach ($blocks as $block): ?>
    <?php print theme('block', $block); ?>
  <?php endforeach; ?>
<?php endif; ?>
