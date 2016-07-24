<?php

namespace Drupal\context_layout\EventSubscriber;

use Drupal\context\ContextManager;
use Drupal\context_layout\Plugin\ContextReaction\Layouts;
use Drupal\Core\Render\RenderEvents;
use Drupal\Core\Render\PageDisplayVariantSelectionEvent;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Selects the layout page display variant.
 *
 * @see \Drupal\block\Plugin\DisplayVariant\BlockPageVariant
 */
class LayoutPageDisplayVariantSubscriber implements EventSubscriberInterface {

  /**
   * Context Manager.
   *
   * @var \Drupal\context\ContextManager
   */
  private $contextManager;

  /**
   * Constructs a LayoutPageDisplayVariantSubscriber object.
   *
   * @param \Drupal\context\ContextManager $contextManager
   *    Context manager service.
   */
  public function __construct(ContextManager $contextManager) {
    $this->contextManager = $contextManager;
  }

  /**
   * Selects the context layout page display variant.
   *
   * @param \Drupal\Core\Render\PageDisplayVariantSelectionEvent $event
   *   The event to process.
   */
  public function onSelectPageDisplayVariant(PageDisplayVariantSelectionEvent $event) {
    // Exit if on admin route and context_layout.settings.admin_allow is true.
    $admin = \Drupal::service('router.admin_context')->isAdminRoute();
    $allow = \Drupal::config('context_layout.settings')
      ->get('admin_allow');
    if ($admin && !$allow) {
      return;
    }
    // Activate the context block page display variant if any of the reactions
    // is a blocks reaction.
    foreach ($this->contextManager->getActiveReactions() as $reaction) {
      if ($reaction instanceof Layouts) {
        $event->setPluginId('context_layout_page');
        break;
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    $events[RenderEvents::SELECT_PAGE_DISPLAY_VARIANT][] = array('onSelectPageDisplayVariant');
    return $events;
  }

}
