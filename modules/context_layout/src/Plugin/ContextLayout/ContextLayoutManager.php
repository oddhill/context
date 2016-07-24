<?php

namespace Drupal\context_layout\Plugin\ContextLayout;

use Drupal\Core\Cache\CacheBackendInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Extension\ThemeHandlerInterface;
use Drupal\layout_plugin\Plugin\Layout\LayoutPluginManager;

/**
 * Provides an interface for the discovery and instantiation of context layouts.
 */
class ContextLayoutManager extends LayoutPluginManager {

  /**
   * {@inheritdoc}
   */
  public function getLayoutOptions(array $params = []) {
    $type = 'full';
    $group_by_category = !empty($params['group_by_category']);
    $plugins = $this->getDefinitions();
    $options = array();
    // Sort the plugins first by category, then by label.
    foreach ($plugins as $id => $plugin) {
      // Only layouts of type 'full' are allowed.
      if ($type != $plugin['type']) {
        continue;
      }
      if ($group_by_category) {
        $category = isset($plugin['category']) ? (string) $plugin['category'] : 'default';
        if (!isset($options[$category])) {
          $options[$category] = array();
        }
        $options[$category][$id] = $plugin['label'];
      }
      else {
        $options[$id] = $plugin['label'];
      }
    }
    return $options;
  }

  /**
   * Returns a Drupal\layout_plugin\Layout instance.
   *
   * @param string $layout
   *    Layout ID (machine name).
   * @param bool|false $fallback
   *    Whether to return a fallback layout if default doesn't exist.
   *
   * @return object
   *    Drupal\layout_plugin\Layout instance.
   */
  public function loadLayout($layout, $fallback = FALSE) {
    // We want to return the correct layout if 'default' is passed.
    if ('default' == $layout) {
      $layout = $this->createInstance($this->getDefaultLayout($fallback));
    }
    else {
      $layout = $this->createInstance($layout);
    }
    return $layout;
  }

  /**
   * Returns default Drupal\layout_plugin\Layout instance.
   *
   * @param bool|false $fallback
   *    Whether to return a fallback layout if default doesn't exist.
   *
   * @return string
   *    Layout ID (machine name).
   */
  public function getDefaultLayout($fallback = FALSE) {
    $layout = \Drupal::config('context_layout.settings')
      ->get('default_layout');
    if ($fallback && !$layout) {
      // Get the first available layout.
      $layout = array_keys(
        $this->getLayoutOptions()
      )[0];
    }
    return $layout;
  }

  /**
   * Return available layout regions.
   *
   * @param array $regions
   *    Region ID's.
   * @param string $layout_id
   *    Layout ID (machine name).
   *
   * @return array
   *    Available layout region ID's.
   */
  public function filterLayoutRegions($regions, $layout_id) {
    $layout = \Drupal::service('plugin.manager.context_layout')
      ->loadLayout($layout_id);
    $layout_regions = array_keys($layout->getRegionDefinitions());
    foreach ($regions as $region_id => $region) {
      if (!in_array($region_id, $layout_regions)) {
        unset($regions[$region_id]);
      }
    }
    return $regions;
  }

}
